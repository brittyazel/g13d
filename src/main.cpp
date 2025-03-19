//
// Created by Britt Yazel on 03-16-2025.
//

#include <csignal>
#include <getopt.h>
#include <iomanip>
#include <thread>

#include "lifecycle.hpp"
#include "key.hpp"
#include "log.hpp"
#include "main.hpp"

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

    bool running;

    void Initialize(const int argc, char* argv[]) {
        InitKeynames();

        start_logging();
        SetLogLevel("INFO");

        //PROJECT_VERSION is defined in meson.build and passed to the compiler
        G13_OUT("g13d v" << PROJECT_VERSION << " " << __DATE__ << " " << __TIME__);

        // TODO: move out argument parsing
        const option long_opts[] = {
                {"logo", required_argument, nullptr, 'l'},
                {"config", required_argument, nullptr, 'c'},
                {"pipe_dir", required_argument, nullptr, 'p'},
                {"umask", required_argument, nullptr, 'u'},
                {"log_level", required_argument, nullptr, 'd'},
                // {"log_file", required_argument, nullptr, 'f'},
                {"help", no_argument, nullptr, 'h'},
                {nullptr, no_argument, nullptr, 0}
            };

        while (true) {
            const auto short_opts = "l:c:p:u:d:h";
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

            case 'p':
                setStringConfigValue("pipe_dir", std::string(optarg));
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

        // Deregister hotplug callbacks
        for (const auto this_handle : usb_hotplug_cb_handle) {
            if (this_handle) {
                libusb_hotplug_deregister_callback(usb_context, this_handle);
            }
        }

        // Cleanup G13 devices
        CleanupDevices();

        // Free device list if allocated
        if (devs) {
            libusb_free_device_list(devs, 1);
            devs = nullptr;
        }

        // Exit libusb context
        if (usb_context) {
            libusb_exit(usb_context);
            usb_context = nullptr;
        }

        // Stop logging
        stop_logging();
    }

    void printHelp() {
        constexpr auto indent = 24;
        std::cout << "Allowed options" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --help" << "produce help message" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --logo <file>" << "set logo from file" << std::endl;
        std::cout << std::left << std::setw(indent) << "  --config <file>" << "load config commands from file" <<
            std::endl;
        std::cout << std::left << std::setw(indent) << "  --pipe_dir <name>" << "specify the root directory for input and output pipes" <<
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
            int ret = InitializeDevices();

            if (g13s.empty() || ret != 0) {
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
            SetupDevice(g13);
        }

        std::thread suspend_thread(MonitorSuspendResume);
        suspend_thread.detach(); // Run the suspend monitoring in the background

        while (running) {
            if (g13s.empty()) {
                G13_OUT("Waiting for device to show up ...");
                error = libusb_handle_events(usb_context);
                G13_OUT("USB Event wakeup with " << g13s.size() << " devices registered");

                if (error != LIBUSB_SUCCESS) {
                    G13_ERR("Error: " << G13_Device::DescribeLibusbErrorCode(error));
                }
                else {
                    for (const auto g13 : g13s) {
                        SetupDevice(g13);
                    }
                }
            }

            // Main loop
            if (!suspended) {
                for (const auto g13 : g13s) {
                    const int status = g13->ReadDeviceInputs();
                    if (!g13s.empty()) {
                        g13->ReadCommandsFromPipe();
                    }
                    if (status < 0) {
                        running = false;
                    }
                }
            }
        }

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
}
