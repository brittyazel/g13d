//
// Created by Britt Yazel on 03-16-2025.
//

#include <memory>
#include <systemd/sd-bus.h>

#include "device.hpp"
#include "lifecycle.hpp"
#include "log.hpp"
#include "main.hpp"


// *************************************************************************

namespace G13 {
    bool suspended = false;

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
        if (error == LIBUSB_ERROR_BUSY && libusb_kernel_driver_active(usb_handle, 0) == 1) {
            if (libusb_detach_kernel_driver(usb_handle, 0) == 0) {
                G13_ERR("Kernel driver detached");
                error = libusb_claim_interface(usb_handle, 0);
            }
        }

        if (error != LIBUSB_SUCCESS) {
            G13_ERR("Cannot Claim Interface: " << G13_Device::DescribeLibusbErrorCode(error));
            libusb_close(usb_handle);
            return 1;
        }

        G13_DBG("Interface successfully claimed");
        const auto g13 = new G13_Device(dev, usb_context, usb_handle, static_cast<int>(g13s.size()));
        g13s.push_back(g13);
        return 0;
    }

    int LIBUSB_CALL HotplugCallbackEnumerate(libusb_context* usb_context, libusb_device* dev,
                                             libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device found during enumeration");
        return HotplugCallbackInsert(usb_context, dev, event, user_data);
    }

    int LIBUSB_CALL HotplugCallbackInsert(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device connected");
        const int ret = InitializeDevices(dev);
        G13_Device::GetG13DeviceHandle(dev)->connected = true;
        return ret; // Rearm
    }

    int LIBUSB_CALL HotplugCallbackRemove(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data) {
        G13_OUT("USB device disconnected");
        G13_Device::GetG13DeviceHandle(dev)->connected = false;
        CleanupDevices(dev);
        return 0; // Rearm
    }

    void SetupDevice(G13_Device* g13) {
        G13_OUT("Setting up device" << " " << g13->getDeviceIndex());
        g13->RegisterContext(usb_context);
        if (!logoFilename.empty()) {
            g13->getLCDRef().LcdWriteFile(logoFilename);
        }

        G13_OUT("Active Stick Zones:");
        g13->getStickRef().dump(std::cout);

        if (const std::string config_filename = getStringConfigValue("config"); !config_filename.empty()) {
            G13_OUT("Reading configuration from: " << config_filename);
            g13->ReadCommandsFromFile(config_filename, "  cfg");
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

    // Cleanup all devices or only the one specified
    void CleanupDevices(const libusb_device* dev) {
        for (auto iter = g13s.begin(); iter != g13s.end();) {
            if (!dev || dev == (*iter)->getDevicePtr()) {
                G13_OUT("Closing device " << std::distance(g13s.begin(), iter));
                (*iter)->Cleanup();
                delete *iter;
                iter = g13s.erase(iter);
            }
            else {
                ++iter;
            }
        }
    }

    // Reinitialize all devices or only the one specified
    int InitializeDevices(libusb_device* dev) {
        if (dev) {
            for (const auto g13 : g13s) {
                if (dev == g13->getDevicePtr()) {
                    return 1;
                }
            }
            OpenAndAddG13(dev);
        }
        else {
            libusb_device** devs;
            if (const ssize_t count = libusb_get_device_list(usb_context, &devs); count < 0) {
                G13_ERR("Failed to get device list");
                return 1;
            }
            else {
                DiscoverG13s(devs, count);
                libusb_free_device_list(devs, 1);
            }
        }

        return 0;
    }

    // Monitor system suspend/resume events using libsystemd
    // This runs in a separate thread started in Run()
    void MonitorSuspendResume() {
        sd_bus* bus = nullptr;
        int ret = sd_bus_open_system(&bus);
        if (ret < 0) {
            G13_OUT("Failed to connect to system bus: " << strerror(-ret));
            return;
        }

        ret = sd_bus_add_match(
            bus, nullptr, "type='signal',interface='org.freedesktop.login1.Manager',member='PrepareForSleep'",
            [](sd_bus_message* m, void* /*userdata*/, sd_bus_error* /*ret_error*/) -> int {
                int suspend_state;
                sd_bus_message_read(m, "b", &suspend_state);

                if (suspend_state) {
                    if (!suspended) {
                        G13_OUT("System is suspending...");
                        suspended = true;
                        CleanupDevices();
                    }
                }
                else {
                    if (suspended) {
                        G13_OUT("System has resumed...");
                        suspended = false;
                        InitializeDevices();
                    }
                }

                return 0;
            }, nullptr);

        if (ret < 0) {
            G13_OUT("Failed to add match: " << strerror(-ret));
            sd_bus_unref(bus);
            return;
        }

        while (true) {
            ret = sd_bus_process(bus, nullptr);
            if (ret > 0) continue; // Process pending messages

            ret = sd_bus_wait(bus, static_cast<uint64_t>(-1));
            if (ret < 0) {
                G13_OUT("Failed to wait on bus: " << strerror(-ret));
                break;
            }
        }

        sd_bus_unref(bus);
    }
}
