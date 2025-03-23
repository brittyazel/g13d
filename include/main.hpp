//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef MAIN_HPP
#define MAIN_HPP

#include <libusb-1.0/libusb.h>
#include <map>
#include <string>
#include <vector>

#include "Objects/device.hpp"


namespace G13 {
    constexpr size_t G13_VENDOR_ID = 0x046d;
    constexpr size_t G13_PRODUCT_ID = 0xc21c;
    constexpr size_t G13_REPORT_SIZE = 8;
    constexpr size_t G13_KEY_ENDPOINT = 1;
    constexpr size_t G13_LCD_ENDPOINT = 2;

    static std::map<std::string, std::string> stringConfigValues;

    extern libusb_context* usb_context;
    extern std::vector<G13_Device*> g13s;
    extern libusb_hotplug_callback_handle usb_hotplug_cb_handle[3];
    extern libusb_device** devs;
    extern std::string logoFilename;
    extern const int class_id;


    void Initialize(int argc, char* argv[]);
    void printHelp();
    int Run();
    void Cleanup();
    void setLogoFilename(const std::string& newLogoFilename);
    std::string getStringConfigValue(const std::string& name);
    void setStringConfigValue(const std::string& name, const std::string& value);
    void SignalHandler(int);
}

#endif
