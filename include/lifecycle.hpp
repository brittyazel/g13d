//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef LIFECYCLE_HPP
#define LIFECYCLE_HPP

#include <libusb-1.0/libusb.h>

#include "Objects/Device.hpp"

namespace G13 {
    extern bool suspended;

    void DiscoverG13s(libusb_device** devs, ssize_t count);
    int OpenAndAddG13(libusb_device* dev);
    void SetupDevice(Device* g13);
    void CleanupDevices(const libusb_device* dev = nullptr);
    int InitializeDevices(libusb_device* dev = nullptr);

    void MonitorSuspendResume();

    int LIBUSB_CALL HotplugCallbackEnumerate(libusb_context* usb_context, libusb_device* dev,
                                             libusb_hotplug_event event, void* user_data);
    int LIBUSB_CALL HotplugCallbackInsert(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data);
    int LIBUSB_CALL HotplugCallbackRemove(libusb_context* usb_context, const libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data);
    void ArmHotplugCallbacks();
}

#endif
