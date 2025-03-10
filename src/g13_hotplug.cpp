//
// Created by khampf on 16-05-2020.
//

#include "g13_hotplug.hpp"
#include "g13.hpp"
#include "g13_device.hpp"
#include "g13_manager.hpp"
#include <libevdev-1.0/libevdev/libevdev.h>
#include <log4cpp/OstreamAppender.hh>
#include <memory>

// *************************************************************************

namespace G13 {

void G13::G13_Manager::DiscoverG13s(libusb_device **devs, ssize_t count) {
  for (int i = 0; i < count; i++) {
    libusb_device_descriptor desc;
    int ret = libusb_get_device_descriptor(devs[i], &desc);
    if (ret != LIBUSB_SUCCESS) {
      G13_ERR("Failed to get device descriptor");
      return;
    }
    if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
      OpenAndAddG13(devs[i]);
      for (auto g13 : g13s) {
        SetupDevice(g13);
      }
    }
  }
}

int G13::G13_Manager::OpenAndAddG13(libusb_device *dev) {

  libusb_device_handle *handle;
  int error = libusb_open(dev, &handle);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("Error opening G13 device: "
            << G13_Device::DescribeLibusbErrorCode(error));
    return 1;
  }

  libusb_set_auto_detach_kernel_driver(handle, true);
  error = libusb_claim_interface(handle, 0);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("Cannot Claim Interface: "
            << G13_Device::DescribeLibusbErrorCode(error));
  }
  if (error == LIBUSB_ERROR_BUSY) {
    if (libusb_kernel_driver_active(handle, 0) == 1) {
      if (libusb_detach_kernel_driver(handle, 0) == 0) {
        G13_ERR("Kernel driver detached");
      }
      error = libusb_claim_interface(handle, 0);
      G13_ERR("Still cannot claim Interface: "
              << G13_Device::DescribeLibusbErrorCode(error));
    }
  }

  if (error == LIBUSB_SUCCESS) {
    G13_DBG("Interface successfully claimed");
    auto g13 = new G13_Device(dev, libusbContext, handle, g13s.size());
    g13s.push_back(g13);
    return 0;
  }

  libusb_release_interface(handle, 0);
  libusb_close(handle);
  return 1;
}

int LIBUSB_CALL G13::G13_Manager::HotplugCallbackEnumerate(
    struct libusb_context *ctx, struct libusb_device *dev,
    libusb_hotplug_event event, void *user_data) {

  G13_OUT("USB device found during enumeration");

  // Call this as it would have been detected on connection later
  HotplugCallbackInsert(ctx, dev, event, user_data);
  return 1;
}

int LIBUSB_CALL G13::G13_Manager::HotplugCallbackInsert(
    struct libusb_context *ctx, struct libusb_device *dev,
    libusb_hotplug_event event, void *user_data) {
  G13_OUT("USB device connected");

  // Just make sure we have not been called multiple times
  for (auto g13 : g13s) {
    if (dev == g13->Device()) {
      return 0;
    }
  }

  // It's brand new!
  OpenAndAddG13(dev);

  // NOTE: can not SetupDevice() from this thread

  return 0; // Rearm
}

int LIBUSB_CALL G13::G13_Manager::HotplugCallbackRemove(
    struct libusb_context *ctx, struct libusb_device *dev,
    libusb_hotplug_event event, void *user_data) {

  G13_OUT("USB device disconnected");
  int i = 0;
  for (auto iter = g13s.begin(); (iter != g13s.end()); ++i) {
    if (dev == (*iter)->Device()) {
      G13_OUT("Closing device " << i);
      auto g13 = iter;
      // (*g13)->Cleanup();
      iter = g13s.erase(iter);  // remove from vector first
      delete (*g13);            // delete the object after
    } else {
      iter++;
    }
  }
  return 0; // Rearm
}

void G13::G13_Manager::SetupDevice(G13_Device *g13) {

  G13_OUT("Setting up device ");
  g13->RegisterContext(libusbContext);
  if (!logoFilename.empty()) {
    g13->LcdWriteFile(logoFilename);
  }

  G13_OUT("Active Stick zones ");
  g13->stick().dump(std::cout);

  std::string config_fn = getStringConfigValue("config");
  if (!config_fn.empty()) {
    G13_OUT("config_fn = " << config_fn);
    g13->ReadConfigFile(config_fn);
  }
}

void G13::G13_Manager::ArmHotplugCallbacks() {
  int error;

  G13_DBG("Registering USB hotplug callbacks");

  // For currently attached devices
  error = libusb_hotplug_register_callback(
      libusbContext, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
      LIBUSB_HOTPLUG_ENUMERATE, G13_VENDOR_ID, G13_PRODUCT_ID, class_id,
      HotplugCallbackEnumerate, nullptr, &hotplug_cb_handle[0]);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("Error registering hotplug enumeration callback: "
            << G13_Device::DescribeLibusbErrorCode(error));
  }

  // For future devices
  error = libusb_hotplug_register_callback(
      libusbContext, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
      LIBUSB_HOTPLUG_NO_FLAGS, G13_VENDOR_ID, G13_PRODUCT_ID, class_id,
      HotplugCallbackInsert, nullptr, &hotplug_cb_handle[1]);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("Error registering hotplug insertion callback: "
            << G13_Device::DescribeLibusbErrorCode(error));
  }

  // For disconnected devices
  error = libusb_hotplug_register_callback(
      libusbContext, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_NO_FLAGS,
      G13_VENDOR_ID, G13_PRODUCT_ID, class_id, HotplugCallbackRemove, nullptr,
      &hotplug_cb_handle[2]);
  if (error != LIBUSB_SUCCESS) {
    G13_ERR("Error registering hotplug removal callback: "
            << G13_Device::DescribeLibusbErrorCode(error));
  }
}

} // namespace G13