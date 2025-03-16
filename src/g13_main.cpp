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
#include "g13_hotplug.hpp"
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

    const char* G13_CommandException::what() const noexcept {
        return reason.c_str();
    }

    void SignalHandler(const int signal) {
        G13_OUT("Caught signal " << signal << " (" << strsignal(signal) << ")");
        running = false;
        // TODO: Should we break libusb handling with a reset?
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

    /****************************************************************/

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

    void setLogoFilename(const std::string& newLogoFilename) {
        logoFilename = newLogoFilename;
    }

} // namespace G13
