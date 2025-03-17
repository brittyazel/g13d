//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef G13_MAIN_HPP_
#define G13_MAIN_HPP_

#include <libusb-1.0/libusb.h>
#include <log4cpp/Priority.hh>

#include "g13_device.hpp"


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
    std::string MakePipeName(const G13_Device* usb_device, bool is_input);
    void SignalHandler(int);


    // *************************************************************************
    class G13_CommandException final : public std::exception {
    public:
        explicit G13_CommandException(std::string reason) : reason(std::move(reason)) {}
        [[nodiscard]] const char* what() const noexcept override;

    private:
        std::string reason;
    };
}

#endif  // G13_MAIN_HPP_
