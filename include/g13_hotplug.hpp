//
// Created by khampf on 16-05-2020.
//

#ifndef G13_HOTPLUG_HPP
#define G13_HOTPLUG_HPP

namespace G13 {

    void DiscoverG13s(libusb_device** devs, ssize_t count);
    int OpenAndAddG13(libusb_device* dev);
    void SetupDevice(G13_Device* g13);

    int LIBUSB_CALL HotplugCallbackEnumerate(libusb_context* usb_context, libusb_device* dev,
                                         libusb_hotplug_event event, void* user_data);
    int LIBUSB_CALL HotplugCallbackInsert(libusb_context* usb_context, libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data);
    int LIBUSB_CALL HotplugCallbackRemove(libusb_context* usb_context, const libusb_device* dev,
                                          libusb_hotplug_event event, void* user_data);
    void ArmHotplugCallbacks();

}

#endif //G13_HOTPLUG_HPP
