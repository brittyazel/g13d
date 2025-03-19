//
// Created by Britt Yazel on 03-16-2025.
//

#include <filesystem>
#include <fstream>
#include <ranges>

#include "action_command.hpp"
#include "action_keys.hpp"
#include "action_pipeout.hpp"
#include "device.hpp"
#include "font.hpp"
#include "font_family.hpp"
#include "key.hpp"
#include "log.hpp"
#include "main.hpp"
#include "stickzone.hpp"
#include "logo.hpp"

namespace G13 {
    // *************************************************************************

    // Constructor
    G13_Device::G13_Device(libusb_device* usb_device, libusb_context* usb_context, libusb_device_handle* usb_handle,
                           const int device_index) : device_index(device_index), usb_context(usb_context),
                                                     uinput_fid(-1), lcd(*this), stick(*this),
                                                     usb_handle(usb_handle), usb_device(usb_device) {
        current_profile = std::make_shared<G13_Profile>(*this, "default");
        profiles["default"] = current_profile;

        for (bool& key : keys) {
            key = false;
        }

        getLCDRef().image_clear();

        InitFonts();
        InitCommands();
    }

    // Destructor
    G13_Device::~G13_Device() {
        Cleanup();
    }

    // *************************************************************************

    void G13_Device::Cleanup() {
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

    void G13_Device::RegisterContext(libusb_context* new_usb_context) {
        usb_context = new_usb_context;

        constexpr int leds = 0;
        constexpr int red = 0;
        constexpr int green = 0;
        constexpr int blue = 255;
        LcdInit();

        SetModeLeds(leds);
        SetKeyColor(red, green, blue);

        uinput_fid = G13CreateUinput(this);
        MakePipeNames();
        input_pipe_fid = G13CreateFifo(input_pipe_name.c_str(), S_IRGRP | S_IROTH);

        if (input_pipe_fid == -1) {
            G13_ERR("failed opening input pipe " << input_pipe_name);
        }

        output_pipe_fid = G13CreateFifo(output_pipe_name.c_str(), S_IWGRP | S_IWOTH);

        if (output_pipe_fid == -1) {
            G13_ERR("failed opening output pipe " << output_pipe_name);
        }
    }

    void G13_Device::MakePipeNames() {
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
    int G13_Device::ReadDeviceInputs() {
        unsigned char buffer[G13_REPORT_SIZE];
        int size = 0;
        const int error = libusb_interrupt_transfer(usb_handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer,
                                                    G13_REPORT_SIZE, &size, 100);

        if (error && error != LIBUSB_ERROR_TIMEOUT) {
            G13_ERR("Error while reading keys: " << DescribeLibusbErrorCode(error));
            if (error == LIBUSB_ERROR_NO_DEVICE || error == LIBUSB_ERROR_IO) {
                G13_DBG("Giving libusb a nudge");
                libusb_handle_events(usb_context);
            }
        }
        if (size == G13_REPORT_SIZE) {
            parse_joystick(buffer);
            getCurrentProfileRef().ParseKeys(buffer);
            SendEvent(EV_SYN, SYN_REPORT, 0);
        }
        return 0;
    }

    bool G13_Device::updateKeyState(const int key, const bool state) {
        // state == true if key is pressed
        const bool oldState = keys[key];
        keys[key] = state;
        return oldState != state;
    }

    int G13_Device::getDeviceIndex() const {
        return device_index;
    }

    G13_LCD& G13_Device::getLCDRef() {
        return lcd;
    }

    G13_Stick& G13_Device::getStickRef() {
        return stick;
    }

    G13_Font& G13_Device::getCurrentFontRef() const {
        return *current_font;
    }

    G13_Profile& G13_Device::getCurrentProfileRef() const {
        return *current_profile;
    }

    libusb_device* G13_Device::getDevicePtr() const {
        return usb_device;
    }

    libusb_device_handle* G13_Device::getHandlePtr() const {
        return usb_handle;
    }

    std::string G13_Device::DescribeLibusbErrorCode(const int code) {
        auto description = std::string(libusb_strerror(code));
        return description;
    }

    int G13CreateFifo(const char* fifo_name, mode_t umask) {
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

    int G13CreateUinput(G13_Device* g13) {
        uinput_user_dev new_uinput{};
        const char* dev_uinput_filename = access("/dev/input/uinput", F_OK) == 0
                                              ? "/dev/input/uinput"
                                              : access("/dev/uinput", F_OK) == 0
                                              ? "/dev/uinput"
                                              : nullptr;
        if (!dev_uinput_filename) {
            G13_ERR("Could not find an uinput device");
            return -1;
        }
        if (access(dev_uinput_filename, W_OK) != 0) {
            G13_ERR(dev_uinput_filename << " doesn't grant write permissions");
            return -1;
        }
        const int ufile = open(dev_uinput_filename, O_WRONLY | O_NDELAY);
        if (ufile <= 0) {
            G13_ERR("Could not open uinput");
            return -1;
        }
        memset(&new_uinput, 0, sizeof(new_uinput));
        constexpr char name[] = "G13";
        memcpy(new_uinput.name, name, sizeof(name));
        new_uinput.id.version = 1;
        new_uinput.id.bustype = BUS_USB;
        new_uinput.id.product = G13_PRODUCT_ID;
        new_uinput.id.vendor = G13_VENDOR_ID;
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
            G13_ERR("Could not write to uinput device (" << return_code << ")");
            return -1;
        }
        return_code = ioctl(ufile, UI_DEV_CREATE);
        if (return_code) {
            G13_ERR("Error creating uinput device for G13");
            return -1;
        }
        return ufile;
    }

    // *************************************************************************

    void G13_Device::SendEvent(const int type, const int code, const int val) {
        memset(&device_event, 0, sizeof(device_event));
        gettimeofday(&device_event.time, nullptr);
        device_event.type = type;
        device_event.code = code;
        device_event.value = val;
        IGUR(write(uinput_fid, &device_event, sizeof(device_event)));
    }

    void G13_Device::OutputPipeWrite(const std::string& out) const {
        IGUR(write(output_pipe_fid, out.c_str(), out.size()));
    }

    void G13_Device::SetModeLeds(const int leds) const {
        unsigned char usb_data[] = {5, 0, 0, 0, 0};
        usb_data[1] = leds;
        const int error = libusb_control_transfer(usb_handle, static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
                                                  static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE), 9, 0x305, 0,
                                                  usb_data, 5, 1000);
        if (error != 5) {
            G13_ERR("Problem setting mode LEDs: " + DescribeLibusbErrorCode(error));
        }
    }

    void G13_Device::SetKeyColor(const int red, const int green, const int blue) const {
        unsigned char usb_data[] = {5, 0, 0, 0, 0};
        usb_data[1] = red;
        usb_data[2] = green;
        usb_data[3] = blue;

        const int error = libusb_control_transfer(usb_handle, static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
                                                  static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE), 9, 0x307, 0,
                                                  usb_data, 5, 1000);
        if (error != 5) {
            G13_ERR("Problem changing color: " + DescribeLibusbErrorCode(error));
        }
    }

    // Normalize and sanitize filename.
    std::string G13_Device::NormalizeFilePath(const std::string& filename) const {
        auto filepath = std::filesystem::path(filename);

        // If relative and loaded from a file, use previous file directory as base.
        if (filepath.is_relative() && !files_currently_loading.empty()) {
            filepath = std::filesystem::path(files_currently_loading.back()).replace_filename(filepath);
        }

        return filepath.lexically_normal().string();
    }

    void G13_Device::ReadCommandsFromFile(const std::string& filename, const char* info) {
        // Normalize and sanitize filename.
        std::string clean_filename = NormalizeFilePath(filename);

        // Check for load recursion.
        if (std::ranges::find(files_currently_loading, clean_filename) != files_currently_loading.end()) {
            G13_ERR(filename << " loading recursion");
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
            G13_LOG(log4cpp::Priority::ERROR << strerror(errno));
            return;
        }

        std::string line;
        while (std::getline(stream, line)) {
            Command(line.c_str(), info);
        }
    }

    void G13_Device::ReadCommandsFromPipe() {
        // Check if data is available to read from the input pipe
        if (IsDataAvailable(input_pipe_fid)) {
            // Copy existing data from the input pipe FIFO buffer
            const auto buffer_end = static_cast<int>(input_pipe_fifo.length());
            char buffer[1024 * 1024];
            memcpy(buffer, input_pipe_fifo.c_str(), buffer_end);

            // Read new data from the input pipe
            const int read_result = static_cast<int>(read(input_pipe_fid, buffer + buffer_end,
                                                          sizeof(buffer) - buffer_end));
            G13_LOG(log4cpp::Priority::DEBUG << "read " << read_result << " characters");

            // If read error occurs, return
            if (read_result < 0) {
                return;
            }

            // Process the buffer containing the read data
            ProcessBuffer(buffer, buffer_end, read_result);
        }
    }

    bool G13_Device::IsDataAvailable(const int fd) {
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

    void G13_Device::ProcessBuffer(char* buffer, int buffer_end, int read_result) {
        // Check if the buffer contains an image
        if (read_result + buffer_end == 960) {
            // Assume the buffer contains an image and send it to the LCD
            getLCDRef().Image(reinterpret_cast<unsigned char*>(buffer), read_result + buffer_end);
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

    std::shared_ptr<G13_Font> G13_Device::SwitchToFont(const std::string& name) {
        std::shared_ptr<G13_Font> font = fonts[name];
        if (font) {
            current_font = font;
        }
        return font;
    }

    void G13_Device::SwitchToProfile(const std::string& name) {
        current_profile = Profile(name);
    }

    std::vector<std::string> G13_Device::FilteredProfileNames(const std::regex& pattern) const {
        std::vector<std::string> names;

        for (const auto& profile_name : profiles | std::views::keys) {
            if (std::regex_match(profile_name, pattern)) {
                names.emplace_back(profile_name);
            }
        }
        return names;
    }

    std::shared_ptr<G13_Profile> G13_Device::Profile(const std::string& name) {
        // try to get profile from map
        std::shared_ptr<G13_Profile> profile = profiles[name];

        // if not found, create it
        if (!profile) {
            profile = std::make_shared<G13_Profile>(*current_profile, name);
            profiles[name] = profile;
        }

        return profile;
    }

    std::shared_ptr<G13_Action> G13_Device::MakeAction(const std::string& action) {
        if (action.empty()) {
            throw G13_CommandException("empty action string");
        }
        if (action[0] == '>') {
            return std::make_shared<G13_Action_PipeOut>(*this, &action[1]);
        }
        if (action[0] == '!') {
            return std::make_shared<G13_Action_Command>(*this, &action[1]);
        }
        return std::make_shared<G13_Action_Keys>(*this, action);
    }

    // *************************************************************************

    void G13_Device::Dump(std::ostream& o, const int detail) {
        o << "G13 id=" << getDeviceIndex() << std::endl;
        o << "   input_pipe_name=" << repr(input_pipe_name) << std::endl;
        o << "   output_pipe_name=" << repr(output_pipe_name) << std::endl;
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

    void G13_Device::InitCommands() {
        // Command to write a string to the LCD
        command_table["out"] = [this](const char* remainder) {
            getLCDRef().WriteString(remainder);
        };

        // Command to set the cursor position on the LCD
        command_table["pos"] = [this](const char* remainder) {
            char* endptr;
            const int row = static_cast<int>(strtol(remainder, &endptr, 10));
            const int col = static_cast<int>(strtol(endptr, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("Bad pos : " << remainder);
            }
            else {
                getLCDRef().WritePos(row, col);
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
                    G13_ERR("Bind key " << keyname << " unknown");
                    return;
                }
                G13_LOG(log4cpp::Priority::DEBUG << "Bind " << keyname << " [" << action << "]");
            }
            catch (const std::exception& ex) {
                G13_ERR("Bind " << keyname << " " << action << " failed : " << ex.what());
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
                G13_ERR("Bad mod format: <" << remainder << ">");
            }
            else {
                SetModeLeds(leds);
            }
        };

        // Command to set the text mode on the LCD
        command_table["textmode"] = [this](const char* remainder) {
            char* endptr;
            const int textmode = static_cast<int>(strtol(remainder, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("bad textmode format: <" << remainder << ">");
            }
            else {
                getLCDRef().text_mode = textmode;
            }
        };

        // Command to set the RGB color of the keys
        command_table["rgb"] = [this](const char* remainder) {
            char* endptr;
            const int red = static_cast<int>(strtol(remainder, &endptr, 10));
            const int green = static_cast<int>(strtol(endptr, &endptr, 10));
            const int blue = static_cast<int>(strtol(endptr, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("rgb bad format: <" << remainder << ">");
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
            G13_ERR("unknown stick mode : <" << mode << ">");
        };

        // Command to manage stick zones
        command_table["stickzone"] = [this](const char* remainder) {
            const std::string operation = extract_and_advance_token(remainder);
            const std::string zonename = extract_and_advance_token(remainder);

            if (operation == "add") {
                getStickRef().zone(zonename, true);
            }
            else {
                G13_StickZone* zone = getStickRef().zone(zonename);
                if (!zone) {
                    throw G13_CommandException("Unknown stick zone");
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
                        throw G13_CommandException("bad bounds format");
                    }
                    G13_OUT("Setting bounds " << x1 << " " << y1 << " " << x2 << " " << y2);
                    zone->set_bounds(G13_ZoneBounds(x1, y1, x2, y2));
                }
                else if (operation == "del") {
                    getStickRef().RemoveZone(*zone);
                }
                else {
                    G13_ERR("Unknown stickzone operation: <" << operation << ">");
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
                G13_ERR("Unknown dump target: <" << target << ">");
            }
        };

        // Command to set the log level
        command_table["log_level"] = [this](const char* remainder) {
            const std::string level = extract_and_advance_token(remainder);
            SetLogLevel(level);
        };

        // Command to refresh the LCD
        command_table["refresh"] = [this](const char* remainder) {
            getLCDRef().image_send();
        };

        // Command to clear the LCD
        command_table["clear"] = [this](const char* remainder) {
            getLCDRef().image_clear();
            getLCDRef().image_send();
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
                    G13_OUT("Profile " << profile << " deleted");
                    found = true;
                }
            }
            else if (target == "key") {
                for (const auto& key : getCurrentProfileRef().FilteredKeyNames(regex_pattern)) {
                    getCurrentProfileRef().FindKey(key)->set_action(nullptr);
                    G13_OUT("Key " << key << " unbound");
                    found = true;
                }
            }
            else if (target == "zone") {
                for (auto& zone : getStickRef().FilteredZoneNames(regex_pattern)) {
                    getStickRef().RemoveZone(*getStickRef().zone(zone));
                    G13_OUT("stickzone " << zone << " unbound");
                    found = true;
                }
            }
            else {
                G13_ERR("Unknown delete target: <" << target << ">");
                found = true;
            }
            if (!found) {
                G13_OUT("No " << target << " name matches <" << glob_pattern << ">");
            }
        };

        // Command to load commands from a file
        command_table["load"] = [this](const char* remainder) {
            const std::string filename = extract_and_advance_token(remainder);
            ReadCommandsFromFile(filename, std::string(1 + files_currently_loading.size(), '>').c_str());
        };
    }

    void G13_Device::Command(const char* str, const char* info) {
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
                G13_OUT(info << ": " << left_trim(str));
            }

            if (command_iter == command_table.end()) {
                G13_ERR("unknown command : " << cmd);
                return;
            }

            // Get the command function
            const COMMAND_FUNCTION& func = command_iter->second;
            // Execute the command function with the remainder of the string
            func(remainder);
        }
        catch (const std::exception& ex) {
            G13_ERR("command failed : " << ex.what());
        }
    }

    void G13_Device::parse_joystick(const unsigned char* buf) {
        getStickRef().ParseJoystick(buf);
    }

    void G13_Device::LcdInit() const {
        if (const int error = libusb_control_transfer(usb_handle, 0, 9, 1, 0, nullptr, 0, 1000); error !=
            LIBUSB_SUCCESS) {
            G13_ERR("Error when initializing LCD endpoint: " << G13_Device::DescribeLibusbErrorCode(error));
        }
        else {
            LcdWrite(g13_logo, sizeof(g13_logo));
        }
    }

    void G13_Device::LcdWrite(const unsigned char* data, const size_t size) const {
        if (size != G13_LCD_BUFFER_SIZE) {
            G13_LOG(
                log4cpp::Priority::ERROR << "Invalid LCD data size " << size << ", should be " << G13_LCD_BUFFER_SIZE);
            return;
        }

        unsigned char buffer[G13_LCD_BUFFER_SIZE + 32] = {};
        buffer[0] = 0x03;
        memcpy(buffer + 32, data, G13_LCD_BUFFER_SIZE);

        int transferred = 0;
        const int error = libusb_interrupt_transfer(usb_handle, LIBUSB_ENDPOINT_OUT | G13_LCD_ENDPOINT, buffer,
                                                    G13_LCD_BUFFER_SIZE + 32, &transferred, 1000);

        if (error) {
            G13_LOG(
                log4cpp::Priority::ERROR << "Error when transferring image: " << DescribeLibusbErrorCode(error) << ", "
                << transferred << " bytes written");
        }
    }

    void G13_Device::LcdWriteFile(const std::string& filename) const {
        std::ifstream filestr;

        filestr.open(filename.c_str());
        std::filebuf* pbuf = filestr.rdbuf();

        const size_t size = pbuf->pubseekoff(0, std::ios::end, std::ios::in);
        pbuf->pubseekpos(0, std::ios::in);

        char buffer[size];

        pbuf->sgetn(buffer, static_cast<long>(size));

        filestr.close();
        LcdWrite(reinterpret_cast<unsigned char*>(buffer), size);
    }

    void G13_Device::InitFonts() {
        current_font = std::make_shared<G13_Font>("8x8", 8);
        fonts[current_font->name()] = current_font;

        current_font->InstallFont(font8x8_basic, G13_FontChar::FF_ROTATE, 0);

        const std::shared_ptr<G13_Font> new_font5x8(new G13_Font("5x8", 5));
        new_font5x8->InstallFont(font5x8_basic, 0, 32);
        fonts[new_font5x8->name()] = new_font5x8;
    }
}
