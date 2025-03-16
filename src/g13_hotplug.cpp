//
// Created by khampf on 16-05-2020.
//

#include <libevdev-1.0/libevdev/libevdev.h>
#include <log4cpp/OstreamAppender.hh>
#include <memory>

#include "g13_device.hpp"
#include "g13_hotplug.hpp"
#include "g13_log.hpp"
#include "g13_main.hpp"


// *************************************************************************

namespace G13 {
    void DiscoverG13s(libusb_device** devs, const ssize_t count) {
        for (int i = 0; i < count; i++) {
            libusb_device_descriptor desc{};
            if (const int ret = libusb_get_device_descriptor(devs[i], &desc); ret != LIBUSB_SUCCESS) {
                G13_ERR("Failed to get device descriptor");
                return;
            }
            if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
                OpenAndAddG13(devs[i]);
                for (const auto g13 : g13s) {
                    SetupDevice(g13);
                }
            }
        }
    }

    int OpenAndAddG13(libusb_device* dev) {
        libusb_device_handle* usb_handle;
        int error = libusb_open(dev, &usb_handle);
        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Error opening G13 device: " << G13_Device::DescribeLibusbErrorCode(error));
            return 1;
        }

        libusb_set_auto_detach_kernel_driver(usb_handle, true);
        error = libusb_claim_interface(usb_handle, 0);
        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Cannot Claim Interface: " << G13_Device::DescribeLibusbErrorCode(error));
        }
        if (error == LIBUSB_ERROR_BUSY) {
            if (libusb_kernel_driver_active(usb_handle, 0) == 1) {
                if (libusb_detach_kernel_driver(usb_handle, 0) == 0) {
                    G13_ERR("Kernel driver detached");
                }
                error = libusb_claim_interface(usb_handle, 0);
                G13_ERR("Still cannot claim Interface: " << G13_Device::DescribeLibusbErrorCode(error));
            }
        }

        if (error == LIBUSB_SUCCESS) {
            G13_DBG("Interface successfully claimed");
            const auto g13 = new G13_Device(dev, usb_context, usb_handle, static_cast<int>(g13s.size()));
            g13s.push_back(g13);
            return 0;
        }

        libusb_release_interface(usb_handle, 0);
        libusb_close(usb_handle);
        return 1;
    }

    int LIBUSB_CALL HotplugCallbackEnumerate(libusb_context* usb_context, libusb_device* dev,
                                             libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device found during enumeration");

        // Call this as it would have been detected on connection later
        HotplugCallbackInsert(usb_context, dev, event, user_data);
        return 1;
    }

    int LIBUSB_CALL HotplugCallbackInsert(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device connected");

        // Just make sure we have not been called multiple times
        for (const auto g13 : g13s) {
            if (dev == g13->getDevicePtr()) {
                return 1;
            }
        }

        // It's brand new!
        OpenAndAddG13(dev);

        // NOTE: can not SetupDevice() from this thread
        return 0; // Rearm
    }

    int LIBUSB_CALL HotplugCallbackRemove(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device disconnected");
        int i = 0;
        for (auto iter = g13s.begin(); iter != g13s.end(); ++i) {
            if (dev == (*iter)->getDevicePtr()) {
                G13_OUT("Closing device " << i);
                const auto g13 = *iter;
                iter = g13s.erase(iter); // remove from vector first
                delete g13; // delete the object after
            }
            else {
                ++iter;
            }
        }
        return 0; // Rearm
    }

    void SetupDevice(G13_Device* g13) {
        G13_OUT("Setting up device ");
        g13->RegisterContext(usb_context);
        if (!logoFilename.empty()) {
            g13->LcdWriteFile(logoFilename);
        }

        G13_OUT("Active Stick zones ");
        g13->getStickRef().dump(std::cout);

        if (const std::string config_fn = getStringConfigValue("config"); !config_fn.empty()) {
            G13_OUT("config_fn = " << config_fn);
            g13->ReadConfigFile(config_fn);
        }
    }

    void ArmHotplugCallbacks() {
        G13_DBG("Registering USB hotplug callbacks");

        // For currently attached devices
        int error = libusb_hotplug_register_callback(usb_context, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                                     LIBUSB_HOTPLUG_ENUMERATE, G13_VENDOR_ID, G13_PRODUCT_ID, class_id,
                                                     HotplugCallbackEnumerate, nullptr, &usb_hotplug_cb_handle[0]);
        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Error registering hotplug enumeration callback: " << G13_Device::DescribeLibusbErrorCode(error));
        }

        // For future devices
        error = libusb_hotplug_register_callback(usb_context, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                                 LIBUSB_HOTPLUG_NO_FLAGS, G13_VENDOR_ID, G13_PRODUCT_ID, class_id,
                                                 HotplugCallbackInsert, nullptr, &usb_hotplug_cb_handle[1]);
        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Error registering hotplug insertion callback: " << G13_Device::DescribeLibusbErrorCode(error));
        }

        // For disconnected devices
        error = libusb_hotplug_register_callback(usb_context, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                                 LIBUSB_HOTPLUG_NO_FLAGS, G13_VENDOR_ID, G13_PRODUCT_ID, class_id,
                                                 HotplugCallbackRemove, nullptr, &usb_hotplug_cb_handle[2]);

        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Error registering hotplug removal callback: " << G13_Device::DescribeLibusbErrorCode(error));
        }
    }
} // namespace G13
