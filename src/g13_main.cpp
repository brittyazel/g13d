#include <csignal>
#include <getopt.h>
#include <iostream>
#include <iomanip>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libusb-1.0/libusb.h>
#include <log4cpp/OstreamAppender.hh>
#include <memory>

#include "g13_main.hpp"
#include "g13_device.hpp"
#include "g13_keys.hpp"
#include "g13_log.hpp"
#include "helper.hpp"
#include "version.hpp"

// *************************************************************************
// ***************************** Entry point *******************************
// *************************************************************************
int main(const int argc, char* argv[]) {
    G13::Initialize(argc, argv);
    return G13::Run();
}

// Main namespace
namespace G13 {
    // definitions
    libusb_context* usb_context = nullptr;
    std::vector<G13_Device*> g13s = {};
    libusb_hotplug_callback_handle usb_hotplug_cb_handle[3] = {};
    libusb_device** devs = nullptr;
    std::string logoFilename;
    const int class_id = LIBUSB_HOTPLUG_MATCH_ANY;

    static std::map<std::string, std::string> stringConfigValues;
    static std::map<G13_KEY_INDEX, std::string> g13_key_to_name;
    static std::map<std::string, G13_KEY_INDEX> g13_name_to_key;
    static std::map<LINUX_KEY_VALUE, std::string> input_key_to_name;
    static std::map<std::string, LINUX_KEY_VALUE> input_name_to_key;
    static LINUX_KEY_VALUE input_key_max;

    static bool running;

    void Initialize(const int argc, char* argv[]) {
        InitKeynames();

        start_logging();
        SetLogLevel("INFO");

        G13_OUT("g13d v" << VERSION_STRING << " " << __DATE__ << " " << __TIME__);

        // TODO: move out argument parsing
        const option long_opts[] = {
            {"logo", required_argument, nullptr, 'l'},
            {"config", required_argument, nullptr, 'c'},
            {"pipe_in", required_argument, nullptr, 'i'},
            {"pipe_out", required_argument, nullptr, 'o'},
            {"umask", required_argument, nullptr, 'u'},
            {"log_level", required_argument, nullptr, 'd'},
            // {"log_file", required_argument, nullptr, 'f'},
            {"help", no_argument, nullptr, 'h'},
            {nullptr, no_argument, nullptr, 0}
        };

        while (true) {
            const auto short_opts = "l:c:i:o:u:d:h";
            const auto opt = getopt_long(argc, argv, short_opts, long_opts, nullptr);

            if (-1 == opt) {
                break;
            }

            switch (opt) {
            case 'l':
                setStringConfigValue("logo", std::string(optarg));
                setLogoFilename(std::string(optarg));
                break;

            case 'c':
                setStringConfigValue("config", std::string(optarg));
                break;

            case 'i':
                setStringConfigValue("pipe_in", std::string(optarg));
                break;

            case 'o':
                setStringConfigValue("pipe_out", std::string(optarg));
                break;

            case 'u':
                setStringConfigValue("umask", std::string(optarg));
                break;

            case 'd':
                setStringConfigValue("log_level", std::string(optarg));
                SetLogLevel(getStringConfigValue("log_level"));
                break;

            case 'h': // -h or --help
            case '?': // Unrecognized option
            default:
                printHelp();
                break;
            }
        }
    }

    void printHelp() {
        constexpr auto indent = 24;
        std::cout << "Allowed options" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --help" << "produce help message" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --logo <file>" << "set logo from file" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --config <file>" << "load config commands from file" <<
            std::endl;
        std::cout << std::left << std::setw(indent) << "  --pipe_in <name>" << "specify name for input pipe" <<
            std::endl;
        std::cout << std::left << std::setw(indent) << "  --pipe_out <name>" << "specify name for output pipe" <<
            std::endl;
        std::cout << std::left << std::setw(indent) << "  --umask <octal>" << "specify umask for pipes creation" <<
            std::endl;
        std::cout << std::left << std::setw(indent) << "  --log_level <level>" << "logging level" << std::endl;
        exit(1);
    }

    void Cleanup() {
        G13_OUT("Cleaning up");
        for (const auto this_handle : usb_hotplug_cb_handle) {
            libusb_hotplug_deregister_callback(usb_context, this_handle);
        }
        // TODO: This might be better with an iterator and also g13s.erase(iter)
        for (const auto g13 : g13s) {
            // g13->Cleanup();
            delete g13;
        }
        libusb_exit(usb_context);
    }

    void InitKeynames() {
        int key_index = 0;

        // setup maps to let us convert between strings and G13 key names
        for (auto name = G13_Key_Tables::G13_KEY_STRINGS; *name; name++) {
            g13_key_to_name[key_index] = *name;
            g13_name_to_key[*name] = key_index;
            G13_DBG("mapping G13 " << *name << " = " << key_index);
            key_index++;
        }

        // setup maps to let us convert between strings and linux key names
        input_key_max = libevdev_event_type_get_max(EV_KEY) + 1;
        for (auto code = 0; code < input_key_max; code++) {
            if (const auto keystroke = libevdev_event_code_get_name(EV_KEY, code); keystroke && !strncmp(
                keystroke, "KEY_", 4)) {
                input_key_to_name[code] = keystroke + 4;
                input_name_to_key[keystroke + 4] = code;
                G13_DBG("mapping " << keystroke + 4 << " " << keystroke << "=" << code);
            }
        }

        // setup maps to let us convert between strings and linux button names
        for (auto symbol = G13_Key_Tables::G13_BTN_SEQ; *symbol; symbol++) {
            auto name = std::string("M" + std::string(*symbol));
            auto keyname = std::string("BTN_" + std::string(*symbol));
            if (int code = libevdev_event_code_from_name(EV_KEY, keyname.c_str()); code < 0) {
                G13_ERR("No input event code found for " << keyname);
            }
            else {
                input_key_to_name[code] = name;
                input_name_to_key[name] = code;
                G13_DBG("mapping " << name << " " << keyname << "=" << code);
            }
        }
    }

    LINUX_KEY_VALUE InputKeyMax() {
        return input_key_max;
    }

    void SignalHandler(const int signal) {
        G13_OUT("Caught signal " << signal << " (" << strsignal(signal) << ")");
        running = false;
        // TODO: Should we break libusb handling with a reset?
    }

    std::string getStringConfigValue(const std::string& name) {
        try {
            return find_or_throw(stringConfigValues, name);
        }
        catch (...) {
            return "";
        }
    }

    void setStringConfigValue(const std::string& name, const std::string& value) {
        G13_DBG("setStringConfigValue " << name << " = " << repr(value));
        stringConfigValues[name] = value;
    }

    std::string MakePipeName(const G13_Device* usb_device, const bool is_input) {
        auto pipe_name = [&](const char* param, const char* suffix) -> std::string {
            if (std::string config_base = getStringConfigValue(param); !config_base.empty()) {
                if (usb_device->getDeviceIndex() == 0) {
                    return config_base;
                }
                return config_base + "-" + std::to_string(usb_device->getDeviceIndex());
            }
            return std::string(CONTROL_DIR) + "/g13-" + std::to_string(usb_device->getDeviceIndex()) + suffix;
        };

        if (is_input) {
            return pipe_name("pipe_in", "");
        }
        return pipe_name("pipe_out", "_out");
    }

    LINUX_KEY_VALUE FindG13KeyValue(const std::string& keyname) {
        const auto i = g13_name_to_key.find(keyname);
        if (i == g13_name_to_key.end()) {
            return BAD_KEY_VALUE;
        }
        return i->second;
    }

    G13_State_Key FindInputKeyValue(const std::string& keyname, bool down) {
        std::string modified_keyname = keyname;

        // If this is a release action, reverse sense
        if (!strncmp(keyname.c_str(), "-", 1)) {
            modified_keyname = keyname.c_str() + 1;
            down = !down;
        }

        // if there is a KEY_ prefix, strip it off
        if (!strncmp(modified_keyname.c_str(), "KEY_", 4)) {
            modified_keyname = modified_keyname.c_str() + 4;
        }

        const auto i = input_name_to_key.find(modified_keyname);
        if (i == input_name_to_key.end()) {
            return G13_State_Key(BAD_KEY_VALUE);
        }
        return G13_State_Key(i->second, down);
    }

    std::string FindInputKeyName(const LINUX_KEY_VALUE v) {
        try {
            return find_or_throw(input_key_to_name, v);
        }
        catch (...) {
            return "(unknown linux key)";
        }
    }

    std::string FindG13KeyName(const G13_KEY_INDEX v) {
        try {
            return find_or_throw(g13_key_to_name, v);
        }
        catch (...) {
            return "(unknown G13 key)";
        }
    }

    void DisplayKeys() {
        G13_OUT("Known keys on G13:");
        G13_OUT(map_keys_out(g13_name_to_key));

        G13_OUT("Known keys to map to:");
        G13_OUT(map_keys_out(input_name_to_key));
    }

    void setLogoFilename(const std::string& newLogoFilename) {
        logoFilename = newLogoFilename;
    }

    int Run() {
        running = true;

        DisplayKeys();

        int error = libusb_init(&usb_context);

        if (error != LIBUSB_SUCCESS) {
            G13_ERR("libusb initialization error: " << G13_Device::DescribeLibusbErrorCode(error));
            Cleanup();
            return EXIT_FAILURE;
        }
        libusb_set_option(usb_context, LIBUSB_OPTION_LOG_LEVEL, 3);

        if (!libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
            const ssize_t cnt = libusb_get_device_list(usb_context, &devs);
            if (cnt < 0) {
                G13_ERR("Error while getting device list");
                Cleanup();
                return EXIT_FAILURE;
            }

            DiscoverG13s(devs, cnt);
            libusb_free_device_list(devs, 1);
            G13_OUT("Found " << g13s.size() << " G13s");
            if (g13s.empty()) {
                G13_ERR("Unable to open any device");
                Cleanup();
                return EXIT_FAILURE;
            }
        }
        else {
            ArmHotplugCallbacks();
        }

        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);

        for (const auto g13 : g13s) {
            // This can not be done from the event handler (will give LIBUSB_ERROR_BUSY)
            SetupDevice(g13);
        }

        do {
            if (g13s.empty()) {
                G13_OUT("Waiting for device to show up ...");
                error = libusb_handle_events(usb_context);
                G13_DBG("USB Event wakeup with " << g13s.size() << " devices registered");
                if (error != LIBUSB_SUCCESS) {
                    G13_ERR("Error: " << G13_Device::DescribeLibusbErrorCode(error));
                }
                else {
                    for (const auto g13 : g13s) {
                        // This can not be done from the event handler (will give LIBUSB_ERROR_BUSY)
                        SetupDevice(g13);
                    }
                }
            }

            // Main loop
            for (const auto g13 : g13s) {
                const int status = g13->ReadKeypresses();
                if (!g13s.empty()) {
                    // Cleanup might have removed the object before this loop has run
                    // TODO: This will not work with multiplt devices and can be better
                    g13->ReadCommandsFromPipe();
                }
                if (status < 0) {
                    running = false;
                }
            }
        }
        while (running);

        Cleanup();
        G13_OUT("Exit");
        return EXIT_SUCCESS;
    }
} // namespace G13
