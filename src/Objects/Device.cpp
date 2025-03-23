//
// Created by Britt Yazel on 03-16-2025.
//

#include <filesystem>
#include <fstream>
#include <ranges>
#include <unistd.h>

#include "Objects/CommandAction.hpp"
#include "Objects/KeyAction.hpp"
#include "Objects/PipeOutAction.hpp"
#include "Objects/Device.hpp"
#include "Assets/logo.hpp"
#include "exceptions.hpp"
#include "Objects/Font.hpp"
#include "Assets/font_family.hpp"
#include "Objects/Key.hpp"
#include "lifecycle.hpp"
#include "log.hpp"
#include "main.hpp"
#include "Objects/StickZone.hpp"

namespace G13 {
    // *************************************************************************

    // Constructor
    Device::Device(libusb_device* usb_device, libusb_context* usb_context, libusb_device_handle* usb_handle,
                           const int device_index) : device_index(device_index), usb_context(usb_context),
                                                     uinput_fid(-1), screen(*this), stick(*this),
                                                     usb_handle(usb_handle), usb_device(usb_device) {
        current_profile = std::make_shared<Profile>(*this, "default");
        profiles["default"] = current_profile;

        connected = true;

        for (bool& key : keys) {
            key = false;
        }

        getScreenRef().image_clear();

        InitFonts();
        InitCommands();
    }

    // Destructor
    Device::~Device() {
        Cleanup();
    }

    // *************************************************************************

    void Device::Cleanup() {
        if (usb_handle) {
            SetKeyColor(0, 0, 0);
            remove(input_pipe_name.c_str());
            remove(output_pipe_name.c_str());
            ioctl(uinput_fid, UI_DEV_DESTROY);
            close(uinput_fid);
            libusb_release_interface(usb_handle, 0);
            libusb_close(usb_handle);
            usb_handle = nullptr;
        }
    }

    void Device::RegisterContext(libusb_context* new_usb_context) {
        usb_context = new_usb_context;

        constexpr int leds = 0;
        constexpr int red = 0;
        constexpr int green = 0;
        constexpr int blue = 255;
        InitScreen();

        SetModeLeds(leds);
        SetKeyColor(red, green, blue);

        uinput_fid = G13CreateUinput();
        MakePipeNames();
        input_pipe_fid = G13CreateFifo(input_pipe_name.c_str(), S_IRGRP | S_IROTH);

        if (input_pipe_fid == -1) {
            ERR("failed opening input pipe " << input_pipe_name);
        }

        output_pipe_fid = G13CreateFifo(output_pipe_name.c_str(), S_IWGRP | S_IWOTH);

        if (output_pipe_fid == -1) {
            ERR("failed opening output pipe " << output_pipe_name);
        }
    }

    void Device::MakePipeNames() {
        if (const std::string config_pipe_dir = getStringConfigValue("pipe_dir"); !config_pipe_dir.empty()) {
            input_pipe_name = config_pipe_dir + "/g13-" + std::to_string(getDeviceIndex());
            output_pipe_name = config_pipe_dir + "/g13-" + std::to_string(getDeviceIndex()) + "_out";
        }
        else {
            // Default to CONTROL_DIR: i.e. /run/g13/g13-0 and /run/g13/g13-0_out
            input_pipe_name = std::string(CONTROL_DIR) + "/g13-" + std::to_string(getDeviceIndex());
            output_pipe_name = std::string(CONTROL_DIR) + "/g13-" + std::to_string(getDeviceIndex()) + "_out";
        }
    }

    // ************************************************************************

    // Reads and processes key state report from G13
    int Device::ReadDeviceInputs() {
        // If not connected or suspended, stop here
        if (!connected || suspended) {
            return 1;
        }

        unsigned char buffer[REPORT_SIZE];
        int size = 0;
        const int error = libusb_interrupt_transfer(usb_handle, LIBUSB_ENDPOINT_IN | KEY_ENDPOINT, buffer,
                                                    REPORT_SIZE, &size, 100);

        if (error && error != LIBUSB_ERROR_TIMEOUT) {
            ERR("Error while reading keys: " << DescribeLibusbErrorCode(error));
            if (error == LIBUSB_ERROR_NO_DEVICE || error == LIBUSB_ERROR_IO) {
                DBG("Giving libusb a nudge");
                libusb_handle_events(usb_context);
            }
        }

        if (size == REPORT_SIZE) {
            getStickRef().ParseJoystick(buffer);
            getCurrentProfileRef().ParseKeys(buffer);
            SendEvent(EV_SYN, SYN_REPORT, 0);
        }
        return 0;
    }

    bool Device::updateKeyState(const int key, const bool state) {
        // state == true if key is pressed
        const bool oldState = keys[key];
        keys[key] = state;
        return oldState != state;
    }

    int Device::getDeviceIndex() const {
        return device_index;
    }

    Screen& Device::getScreenRef() {
        return screen;
    }

    Stick& Device::getStickRef() {
        return stick;
    }

    Font& Device::getCurrentFontRef() const {
        return *current_font;
    }

    Profile& Device::getCurrentProfileRef() const {
        return *current_profile;
    }

    libusb_device* Device::getDevicePtr() const {
        return usb_device;
    }

    libusb_device_handle* Device::getHandlePtr() const {
        return usb_handle;
    }

    Device* Device::GetG13DeviceHandle(const libusb_device* dev) {
        for (const auto g13 : g13s) {
            if (dev == g13->getDevicePtr()) {
                return g13;
            }
        }
        return nullptr;
    }

    std::string Device::DescribeLibusbErrorCode(const int code) {
        auto description = std::string(libusb_strerror(code));
        return description;
    }

    int Device::G13CreateFifo(const char* fifo_name, mode_t umask) {
        // Extract the directory path from the FIFO path
        const std::filesystem::path fifo_path(fifo_name);

        // Create directories recursively if they don't exist
        if (const std::filesystem::path dir_path = fifo_path.parent_path(); !dir_path.empty()) {
            create_directories(dir_path);
        }

        umask |= std::stoi(std::string("0") + getStringConfigValue("umask"), nullptr, 8);
        mkfifo(fifo_name, 0666);
        const int fd = open(fifo_name, O_RDWR | O_NONBLOCK);
        chmod(fifo_name, 0777 & ~umask);
        return fd;
    }

    int Device::G13CreateUinput() {
        uinput_user_dev new_uinput{};
        const char* dev_uinput_filename = access("/dev/input/uinput", F_OK) == 0
                                              ? "/dev/input/uinput"
                                              : access("/dev/uinput", F_OK) == 0
                                              ? "/dev/uinput"
                                              : nullptr;
        if (!dev_uinput_filename) {
            ERR("Could not find an uinput device");
            return -1;
        }
        if (access(dev_uinput_filename, W_OK) != 0) {
            ERR(dev_uinput_filename << " doesn't grant write permissions");
            return -1;
        }
        const int ufile = open(dev_uinput_filename, O_WRONLY | O_NDELAY);
        if (ufile <= 0) {
            ERR("Could not open uinput");
            return -1;
        }
        memset(&new_uinput, 0, sizeof(new_uinput));
        constexpr char name[] = "G13";
        memcpy(new_uinput.name, name, sizeof(name));
        new_uinput.id.version = 1;
        new_uinput.id.bustype = BUS_USB;
        new_uinput.id.product = PRODUCT_ID;
        new_uinput.id.vendor = VENDOR_ID;
        new_uinput.absmin[ABS_X] = 0;
        new_uinput.absmin[ABS_Y] = 0;
        new_uinput.absmax[ABS_X] = 0xff;
        new_uinput.absmax[ABS_Y] = 0xff;

        ioctl(ufile, UI_SET_EVBIT, EV_KEY);
        ioctl(ufile, UI_SET_EVBIT, EV_ABS);
        ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
        ioctl(ufile, UI_SET_ABSBIT, ABS_X);
        ioctl(ufile, UI_SET_ABSBIT, ABS_Y);

        for (int i = 0; i < 256; i++) {
            ioctl(ufile, UI_SET_KEYBIT, i);
        }

        // Mouse buttons
        for (int i = 0x110; i < 0x118; i++) {
            ioctl(ufile, UI_SET_KEYBIT, i);
        }
        ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

        ssize_t return_code = write(ufile, &new_uinput, sizeof(new_uinput));
        if (return_code < 0) {
            ERR("Could not write to uinput device (" << return_code << ")");
            return -1;
        }
        return_code = ioctl(ufile, UI_DEV_CREATE);
        if (return_code) {
            ERR("Error creating uinput device for G13");
            return -1;
        }
        return ufile;
    }

    // *************************************************************************

    void Device::SendEvent(const int type, const int code, const int val) {
        memset(&device_event, 0, sizeof(device_event));
        gettimeofday(&device_event.time, nullptr);
        device_event.type = type;
        device_event.code = code;
        device_event.value = val;
        IGUR(write(uinput_fid, &device_event, sizeof(device_event)));
    }

    void Device::OutputPipeWrite(const std::string& out) const {
        IGUR(write(output_pipe_fid, out.c_str(), out.size()));
    }

    void Device::SetModeLeds(const int leds) const {
        unsigned char usb_data[] = {5, 0, 0, 0, 0};
        usb_data[1] = leds;
        const int error = libusb_control_transfer(usb_handle, static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
                                                  static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE), 9, 0x305, 0,
                                                  usb_data, 5, 1000);
        if (error != 5) {
            ERR("Problem setting mode LEDs: " + DescribeLibusbErrorCode(error));
        }
    }

    void Device::SetKeyColor(const int red, const int green, const int blue) const {
        if (!connected) {
            return;
        }

        unsigned char usb_data[] = {5, 0, 0, 0, 0};
        usb_data[1] = red;
        usb_data[2] = green;
        usb_data[3] = blue;

        const int error = libusb_control_transfer(usb_handle, static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
                                                  static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE), 9, 0x307, 0,
                                                  usb_data, 5, 1000);
        if (error != 5) {
            ERR("Problem changing color: " + DescribeLibusbErrorCode(error));
        }
    }

    // Normalize and sanitize filename.
    std::string Device::NormalizeFilePath(const std::string& filename) const {
        auto filepath = std::filesystem::path(filename);

        // If relative and loaded from a file, use previous file directory as base.
        if (filepath.is_relative() && !files_currently_loading.empty()) {
            filepath = std::filesystem::path(files_currently_loading.back()).replace_filename(filepath);
        }

        return filepath.lexically_normal().string();
    }

    void Device::ReadCommandsFromFile(const std::string& filename, const char* info) {
        // Normalize and sanitize filename.
        std::string clean_filename = NormalizeFilePath(filename);

        // Check for load recursion.
        if (std::ranges::find(files_currently_loading, clean_filename) != files_currently_loading.end()) {
            ERR(filename << " loading recursion");
            return;
        }

        // Add filename to files currently loading
        files_currently_loading.emplace_back(clean_filename);

        // Ensure filename is removed from files currently loading when function exits
        auto remove_filename = [this]() {
            files_currently_loading.pop_back();
        };
        struct ScopeGuard {
            std::function<void()> on_exit;

            ~ScopeGuard() {
                on_exit();
            }
        } guard{remove_filename};

        std::ifstream stream(clean_filename);
        if (!stream) {
            LOG(log4cpp::Priority::ERROR << strerror(errno));
            return;
        }

        std::string line;
        while (std::getline(stream, line)) {
            Command(line.c_str(), info);
        }
    }

    void Device::ReadCommandsFromPipe() {
        // Check if data is available to read from the input pipe
        if (IsDataAvailable(input_pipe_fid)) {
            // Copy existing data from the input pipe FIFO buffer
            const auto buffer_end = static_cast<int>(input_pipe_fifo.length());
            char buffer[1024 * 1024];
            memcpy(buffer, input_pipe_fifo.c_str(), buffer_end);

            // Read new data from the input pipe
            const int read_result = static_cast<int>(read(input_pipe_fid, buffer + buffer_end,
                                                          sizeof(buffer) - buffer_end));
            LOG(log4cpp::Priority::DEBUG << "read " << read_result << " characters");

            // If read error occurs, return
            if (read_result < 0) {
                return;
            }

            // Process the buffer containing the read data
            ProcessBuffer(buffer, buffer_end, read_result);
        }
    }

    bool Device::IsDataAvailable(const int fd) {
        // Set up the file descriptor set for select()
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);

        // Set up the timeout for select()
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        // Check if data is available to read
        return select(fd + 1, &set, nullptr, nullptr, &tv) > 0;
    }

    void Device::ProcessBuffer(char* buffer, int buffer_end, int read_result) {
        // Check if the buffer contains an image
        if (read_result + buffer_end == 960) {
            // Assume the buffer contains an image and send it to the screen
            getScreenRef().Image(reinterpret_cast<unsigned char*>(buffer), read_result + buffer_end);
        }
        else {
            // Process the buffer line by line
            int buffer_begin = 0;
            for (read_result += buffer_end; buffer_end < read_result; buffer_end++) {
                if (buffer[buffer_end] == '\r' || buffer[buffer_end] == '\n') {
                    if (buffer_end != buffer_begin) {
                        buffer[buffer_end] = '\0';
                        Command(buffer + buffer_begin, "command");
                    }
                    buffer_begin = buffer_end + 1;
                }
            }

            // Clear the input pipe FIFO buffer
            input_pipe_fifo.clear();

            // If there is remaining data, store it in the input pipe FIFO buffer
            if (read_result - buffer_begin < static_cast<int>(sizeof(buffer))) {
                input_pipe_fifo = std::string(buffer + buffer_begin, read_result - buffer_begin);
            }
        }
    }

    std::shared_ptr<Font> Device::SwitchToFont(const std::string& name) {
        std::shared_ptr<Font> font = fonts[name];
        if (font) {
            current_font = font;
        }
        return font;
    }

    std::vector<std::string> Device::FilteredProfileNames(const std::regex& pattern) const {
        std::vector<std::string> names;

        for (const auto& profile_name : profiles | std::views::keys) {
            if (std::regex_match(profile_name, pattern)) {
                names.emplace_back(profile_name);
            }
        }
        return names;
    }

    void Device::SwitchToProfile(const std::string& name) {
        // try to get profile from map
        auto profile = profiles[name];

        // if not found, create it
        if (!profile) {
            profile = std::make_shared<Profile>(*current_profile, name);
            profiles[name] = profile;
        }

        current_profile = profile;
    }

    std::shared_ptr<Action> Device::MakeAction(const std::string& action) {
        if (action.empty()) {
            throw CommandException("empty action string");
        }
        if (action[0] == '>') {
            return std::make_shared<PipeOutAction>(*this, &action[1]);
        }
        if (action[0] == '!') {
            return std::make_shared<CommandAction>(*this, &action[1]);
        }
        return std::make_shared<KeyAction>(*this, action);
    }

    // *************************************************************************

    void Device::InitFonts() {
        current_font = std::make_shared<Font>("8x8", 8);
        fonts[current_font->name()] = current_font;

        current_font->InstallFont(font_basic_large, FontCharacter::FF_ROTATE, 0);

        const std::shared_ptr<Font> new_font5x8(new Font("5x8", 5));
        new_font5x8->InstallFont(font_basic_small, 0, 32);
        fonts[new_font5x8->name()] = new_font5x8;
    }

    // Initialization
    void Device::InitScreen() {
        if (const int error = libusb_control_transfer(getHandlePtr(), 0, 9, 1, 0, nullptr, 0, 1000);
            error != LIBUSB_SUCCESS) {
            ERR("Error when initializing screen endpoint: " << DescribeLibusbErrorCode(error));
        }
        else {
            getScreenRef().ScreenWrite(logo, sizeof(logo));
        }
    }

    void Device::InitCommands() {
        // Command to write a string to the screen
        command_table["out"] = [this](const char* remainder) {
            getScreenRef().WriteString(remainder);
        };

        // Command to set the cursor position on the screen
        command_table["pos"] = [this](const char* remainder) {
            char* endptr;
            const int row = static_cast<int>(strtol(remainder, &endptr, 10));
            const int col = static_cast<int>(strtol(endptr, &endptr, 10));

            if (*endptr != '\0') {
                ERR("Bad pos : " << remainder);
            }
            else {
                getScreenRef().WritePos(row, col);
            }
        };

        // Command to bind a key or stick zone to an action
        command_table["bind"] = [this](const char* remainder) {
            const std::string keyname = extract_and_advance_token(remainder);
            const char* raw_action = left_trim(remainder);
            std::string action = extract_and_advance_token(remainder);
            const std::string action_up = extract_and_advance_token(remainder);

            if (!action.empty() && strchr("!>", action[0])) {
                action = std::string(raw_action);
            }
            else if (!action_up.empty()) {
                action += std::string(" ") + action_up;
            }

            try {
                if (const auto key = getCurrentProfileRef().FindKey(keyname)) {
                    key->set_action(MakeAction(action));
                }
                else if (const auto stick_key = getStickRef().zone(keyname)) {
                    stick_key->set_action(MakeAction(action));
                }
                else {
                    ERR("Bind key " << keyname << " unknown");
                    return;
                }
                LOG(log4cpp::Priority::DEBUG << "Bind " << keyname << " [" << action << "]");
            }
            catch (const std::exception& ex) {
                ERR("Bind " << keyname << " " << action << " failed : " << ex.what());
            }
        };

        // Command to switch to a different profile
        command_table["profile"] = [this](const char* remainder) {
            const std::string profile = extract_and_advance_token(remainder);
            SwitchToProfile(profile);
        };

        // Command to switch to a different font
        command_table["font"] = [this](const char* remainder) {
            const std::string font = extract_and_advance_token(remainder);
            SwitchToFont(font);
        };

        // Command to set the mode LEDs
        command_table["mod"] = [this](const char* remainder) {
            char* endptr;
            const int leds = static_cast<int>(strtol(remainder, &endptr, 10));

            if (*endptr != '\0') {
                ERR("Bad mod format: <" << remainder << ">");
            }
            else {
                SetModeLeds(leds);
            }
        };

        // Command to set the text mode on the screen
        command_table["textmode"] = [this](const char* remainder) {
            char* endptr;
            const int textmode = static_cast<int>(strtol(remainder, &endptr, 10));

            if (*endptr != '\0') {
                ERR("bad textmode format: <" << remainder << ">");
            }
            else {
                getScreenRef().setTextMode(textmode);
            }
        };

        // Command to set the RGB color of the keys
        command_table["rgb"] = [this](const char* remainder) {
            char* endptr;
            const int red = static_cast<int>(strtol(remainder, &endptr, 10));
            const int green = static_cast<int>(strtol(endptr, &endptr, 10));
            const int blue = static_cast<int>(strtol(endptr, &endptr, 10));

            if (*endptr != '\0') {
                ERR("rgb bad format: <" << remainder << ">");
            }
            else {
                SetKeyColor(red, green, blue);
            }
        };

        // Command to set the stick mode
        command_table["stickmode"] = [this](const char* remainder) {
            const std::string mode = extract_and_advance_token(remainder);

            const std::string modes[] = {"ABSOLUTE", "KEYS", "CALCENTER", "CALBOUNDS", "CALNORTH"};
            int index = 0;
            for (auto& test : modes) {
                if (test == mode) {
                    getStickRef().set_mode(static_cast<stick_mode_t>(index));
                    return;
                }
                index++;
            }
            ERR("unknown stick mode : <" << mode << ">");
        };

        // Command to manage stick zones
        command_table["stickzone"] = [this](const char* remainder) {
            const std::string operation = extract_and_advance_token(remainder);
            const std::string zonename = extract_and_advance_token(remainder);

            if (operation == "add") {
                getStickRef().zone(zonename, true);
            }
            else {
                StickZone* zone = getStickRef().zone(zonename);
                if (!zone) {
                    throw CommandException("Unknown stick zone");
                }
                if (operation == "action") {
                    zone->set_action(MakeAction(remainder));
                }
                else if (operation == "bounds") {
                    char* endptr;
                    const double x1 = strtod(remainder, &endptr);
                    const double y1 = strtod(endptr, &endptr);
                    const double x2 = strtod(endptr, &endptr);
                    const double y2 = strtod(endptr, nullptr);
                    if (endptr == remainder) {
                        throw CommandException("bad bounds format");
                    }
                    OUT("Setting bounds " << x1 << " " << y1 << " " << x2 << " " << y2);
                    zone->set_bounds(ZoneBounds(x1, y1, x2, y2));
                }
                else if (operation == "del") {
                    getStickRef().RemoveZone(*zone);
                }
                else {
                    ERR("Unknown stickzone operation: <" << operation << ">");
                }
            }
        };

        // Command to dump the current state
        command_table["dump"] = [this](const char* remainder) {
            const std::string target = extract_and_advance_token(remainder);

            if (target == "all") {
                Dump(std::cout, 3);
            }
            else if (target == "current") {
                Dump(std::cout, 1);
            }
            else if (target == "summary") {
                Dump(std::cout, 0);
            }
            else {
                ERR("Unknown dump target: <" << target << ">");
            }
        };

        // Command to set the log level
        command_table["log_level"] = [this](const char* remainder) {
            const std::string level = extract_and_advance_token(remainder);
            SetLogLevel(level);
        };

        // Command to refresh the screen
        command_table["refresh"] = [this](const char* remainder) {
            getScreenRef().image_send();
        };

        // Command to clear the screen
        command_table["clear"] = [this](const char* remainder) {
            getScreenRef().image_clear();
            getScreenRef().image_send();
        };

        // Command to delete profiles, keys, or zones
        command_table["delete"] = [this](const char* remainder) {
            bool found = false;

            const std::string target = extract_and_advance_token(remainder);
            const std::string glob_pattern = extract_and_advance_token(remainder);

            const std::regex regex_pattern(glob_to_regex(glob_pattern.c_str()));

            if (target == "profile") {
                for (auto& profile : FilteredProfileNames(regex_pattern)) {
                    profiles.erase(profile);
                    OUT("Profile " << profile << " deleted");
                    found = true;
                }
            }
            else if (target == "key") {
                for (const auto& key : getCurrentProfileRef().FilteredKeyNames(regex_pattern)) {
                    getCurrentProfileRef().FindKey(key)->set_action(nullptr);
                    OUT("Key " << key << " unbound");
                    found = true;
                }
            }
            else if (target == "zone") {
                for (auto& zone : getStickRef().FilteredZoneNames(regex_pattern)) {
                    getStickRef().RemoveZone(*getStickRef().zone(zone));
                    OUT("stickzone " << zone << " unbound");
                    found = true;
                }
            }
            else {
                ERR("Unknown delete target: <" << target << ">");
                found = true;
            }
            if (!found) {
                OUT("No " << target << " name matches <" << glob_pattern << ">");
            }
        };

        // Command to load commands from a file
        command_table["load"] = [this](const char* remainder) {
            const std::string filename = extract_and_advance_token(remainder);
            ReadCommandsFromFile(filename, std::string(1 + files_currently_loading.size(), '>').c_str());
        };
    }

    void Device::Command(const char* str, const char* info) {
        // Pointer to the remainder of the command string
        const char* remainder = str;

        try {
            // Extract the command from the string
            const std::string cmd = extract_and_advance_token(remainder);

            if (cmd.empty()) {
                return;
            }

            // Find the command in the command table
            const auto command_iter = command_table.find(cmd);
            if (info) {
                OUT(info << ": " << left_trim(str));
            }

            if (command_iter == command_table.end()) {
                ERR("unknown command : " << cmd);
                return;
            }

            // Get the command function
            const COMMAND_FUNCTION& func = command_iter->second;
            // Execute the command function with the remainder of the string
            func(remainder);
        }
        catch (const std::exception& ex) {
            ERR("command failed : " << ex.what());
        }
    }

    void Device::Dump(std::ostream& o, const int detail) {
        o << "G13 id=" << getDeviceIndex() << std::endl;
        o << "   input_pipe_name=" << formatter(input_pipe_name) << std::endl;
        o << "   output_pipe_name=" << formatter(output_pipe_name) << std::endl;
        o << "   current_profile=" << getCurrentProfileRef().name() << std::endl;
        o << "   current_font=" << getCurrentFontRef().name() << std::endl;

        if (detail > 0) {
            o << "STICK" << std::endl;
            getStickRef().dump(o);
            if (detail == 1) {
                getCurrentProfileRef().dump(o);
            }
            else {
                for (const auto& profile : profiles | std::views::values) {
                    profile->dump(o);
                }
            }
        }
    }
}
