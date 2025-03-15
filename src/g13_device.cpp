//
// Created by khampf on 07-05-2020.
//

#include <filesystem>
#include <fstream>
#include <ranges>
#include <regex>
#include <unistd.h>
#include <vector>

#include "g13_device.hpp"
#include "g13_fonts.hpp"
#include "g13_log.hpp"

#include "g13_main.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"

namespace G13 {
    // *************************************************************************

    // Constructor
    G13_Device::G13_Device(libusb_device* usb_device, libusb_context* usb_context, libusb_device_handle* usb_handle, const int device_index)
        : device_index(device_index),
          usb_context(usb_context),
          uinput_fid(-1),
          lcd(*this),
          stick(*this),
          usb_handle(usb_handle),
          usb_device(usb_device) {
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

    bool G13_Device::updateKeyState(const int key, const bool state) {
        // state = true if key is pressed
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

    static int G13CreateFifo(const char* fifo_name, mode_t umask) {
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
        uinput_user_dev uinp{};
        const char* dev_uinput_fname = access("/dev/input/uinput", F_OK) == 0
                                           ? "/dev/input/uinput"
                                           : access("/dev/uinput", F_OK) == 0
                                           ? "/dev/uinput"
                                           : nullptr;
        if (!dev_uinput_fname) {
            G13_ERR("Could not find an uinput device");
            return -1;
        }
        if (access(dev_uinput_fname, W_OK) != 0) {
            G13_ERR(dev_uinput_fname << " doesn't grant write permissions");
            return -1;
        }
        const int ufile = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
        if (ufile <= 0) {
            G13_ERR("Could not open uinput");
            return -1;
        }
        memset(&uinp, 0, sizeof(uinp));
        constexpr char name[] = "G13";
        memcpy(uinp.name, name, sizeof(name));
        uinp.id.version = 1;
        uinp.id.bustype = BUS_USB;
        uinp.id.product = G13_PRODUCT_ID;
        uinp.id.vendor = G13_VENDOR_ID;
        uinp.absmin[ABS_X] = 0;
        uinp.absmin[ABS_Y] = 0;
        uinp.absmax[ABS_X] = 0xff;
        uinp.absmax[ABS_Y] = 0xff;
        //  uinp.absfuzz[ABS_X] = 4;
        //  uinp.absfuzz[ABS_Y] = 4;
        //  uinp.absflat[ABS_X] = 0x80;
        //  uinp.absflat[ABS_Y] = 0x80;

        ioctl(ufile, UI_SET_EVBIT, EV_KEY);
        ioctl(ufile, UI_SET_EVBIT, EV_ABS);
        /*  ioctl(ufile, UI_SET_EVBIT, EV_REL);*/
        ioctl(ufile, UI_SET_MSCBIT, MSC_SCAN);
        ioctl(ufile, UI_SET_ABSBIT, ABS_X);
        ioctl(ufile, UI_SET_ABSBIT, ABS_Y);
        /*  ioctl(ufile, UI_SET_RELBIT, REL_X);
         ioctl(ufile, UI_SET_RELBIT, REL_Y);*/
        for (int i = 0; i < 256; i++) {
            ioctl(ufile, UI_SET_KEYBIT, i);
        }

        // Mouse buttons
        for (int i = 0x110; i < 0x118; i++) {
            ioctl(ufile, UI_SET_KEYBIT, i);
        }
        ioctl(ufile, UI_SET_KEYBIT, BTN_THUMB);

        ssize_t retcode = write(ufile, &uinp, sizeof(uinp));
        if (retcode < 0) {
            G13_ERR("Could not write to uinput device (" << retcode << ")");
            return -1;
        }
        retcode = ioctl(ufile, UI_DEV_CREATE);
        if (retcode) {
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

    /*! reads and processes key state report from G13
     *
     */
    int G13_Device::ReadKeypresses() {
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

    void G13_Device::ReadCommandsFromFile(const std::string& filename, const char* info) {
        class in_use {
        public:
            in_use(G13_Device* device, const std::string& filename) : device(device) {
                device->files_currently_loading.emplace_back(filename);
            }

            ~in_use() {
                device->files_currently_loading.pop_back();
            }

        private:
            G13_Device* device;
        };

        // Normalize and sanitize filename.
        auto filepath = std::filesystem::path(filename);

        // If relative and loaded from a file, use previous file directory as base.
        if (filepath.is_relative() && !files_currently_loading.empty()) {
            filepath = std::filesystem::path(files_currently_loading.back()).replace_filename(filepath);
        }

        filepath = filepath.lexically_normal();
        std::string clean_filename = filepath.string();

        // Check for load recursion.
        if (std::ranges::find(files_currently_loading, clean_filename) != files_currently_loading.end()) {
            G13_ERR(filename << " loading recursion");
            return;
        }

        in_use autoclean(this, clean_filename);

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

    void G13_Device::ReadConfigFile(const std::string& filename) {
        G13_OUT("reading configuration from " << filename);
        ReadCommandsFromFile(filename, "  cfg");
    }

    void G13_Device::ReadCommandsFromPipe() {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(input_pipe_fid, &set);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        if (auto ret = select(input_pipe_fid + 1, &set, nullptr, nullptr, &tv); ret > 0) {
            auto end = static_cast<int>(input_pipe_fifo.length());
            char buf[1024 * 1024];
            memcpy(buf, input_pipe_fifo.c_str(), end);
            ret = static_cast<int>(read(input_pipe_fid, buf + end, sizeof buf - end));
            G13_LOG(log4cpp::Priority::DEBUG << "read " << ret << " characters");

            if (ret < 0) {}
            // Read error: should not occur after successful select().
            else if (ret + end == 960) {
                // TODO probably image, for now, don't test, just assume image
                getLCDRef().Image(reinterpret_cast<unsigned char*>(buf), ret + end);
            }
            else {
                size_t beg = 0;
                for (ret += end; end < static_cast<size_t>(ret); end++) {
                    if (buf[end] == '\r' || buf[end] == '\n') {
                        if (end != beg) {
                            buf[end] = '\0';
                            Command(buf + beg, "command");
                        }
                        beg = end + 1;
                    }
                }
                input_pipe_fifo.clear();
                if (ret - beg < sizeof buf) {
                    // Drop too long lines.
                    input_pipe_fifo = std::string(buf + beg, ret - beg);
                }
            }
        }
    }

    FontPtr G13_Device::SwitchToFont(const std::string& name) {
        FontPtr font = fonts[name];
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

    ProfilePtr G13_Device::Profile(const std::string& name) {
        // try to get profile from map
        ProfilePtr profile = profiles[name];

        // if not found, create it
        if (!profile) {
            profile = std::make_shared<G13_Profile>(*current_profile, name);
            profiles[name] = profile;
        }

        return profile;
    }

    G13_ActionPtr G13_Device::MakeAction(const std::string& action) {
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
        // UNREACHABLE: throw G13_CommandException("can't create action for " +
        // action);
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

    struct commandAdder {
        commandAdder(G13_Device::CommandFunctionTable& t, const char* name) : _t(t), _name(name) {}

        commandAdder(G13_Device::CommandFunctionTable& t, const char* name,
                     G13_Device::COMMAND_FUNCTION f) : _t(t), _name(name) {
            _t[_name] = std::move(f);
        }

        G13_Device::CommandFunctionTable& _t;
        std::string _name;

        commandAdder& operator+=(G13_Device::COMMAND_FUNCTION f) {
            _t[_name] = std::move(f);
            return *this;
        };
    };

    void G13_Device::InitCommands() {

        commandAdder add_out(command_table, "out", [this](const char* remainder) {
            getLCDRef().WriteString(remainder);
        });

        commandAdder add_pos(command_table, "pos", [this](const char* remainder) {
            char* endptr;
            const int row = static_cast<int>(strtol(remainder, &endptr, 10));
            const int col = static_cast<int>(strtol(endptr, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("bad pos : " << remainder);
            }
            else {
                getLCDRef().WritePos(row, col);
            }
        });

        commandAdder add_bind(command_table, "bind", [this](const char* remainder) {
            std::string keyname, action, actionup;
            advance_ws(remainder, keyname);
            const char* rawaction = ltrim(remainder);
            advance_ws(remainder, action);
            advance_ws(remainder, actionup);

            if (!action.empty() && strchr("!>", action[0])) {
                action = std::string(rawaction);
            }
            else if (!actionup.empty()) {
                action += std::string(" ") + actionup;
            }

            try {
                if (const auto key = getCurrentProfileRef().FindKey(keyname)) {
                    key->set_action(MakeAction(action));
                }
                else if (const auto stick_key = getStickRef().zone(keyname)) {
                    stick_key->set_action(MakeAction(action));
                }
                else {
                    G13_ERR("bind key " << keyname << " unknown");
                    return;
                }
                G13_LOG(log4cpp::Priority::DEBUG << "bind " << keyname << " [" << action << "]");
            }
            catch (const std::exception& ex) {
                G13_ERR("bind " << keyname << " " << action << " failed : " << ex.what());
            }
        });

        commandAdder add_profile(command_table, "profile", [this](const char* remainder) {
            std::string profile;
            advance_ws(remainder, profile);
            SwitchToProfile(profile);
        });

        commandAdder add_font(command_table, "font", [this](const char* remainder) {
            std::string font;
            advance_ws(remainder, font);
            SwitchToFont(font);
        });

        commandAdder add_mod(command_table, "mod", [this](const char* remainder) {
            char* endptr;
            const int leds = static_cast<int>(strtol(remainder, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("bad mod format: <" << remainder << ">");
            }
            else {
                SetModeLeds(leds);
            }
        });

        commandAdder add_textmode(command_table, "textmode", [this](const char* remainder) {
            char* endptr;
            const int textmode = static_cast<int>(strtol(remainder, &endptr, 10));

            if (*endptr != '\0') {
                G13_ERR("bad textmode format: <" << remainder << ">");
            }
            else {
                getLCDRef().text_mode = textmode;
            }
        });

        commandAdder add_rgb(command_table, "rgb", [this](const char* remainder) {
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
        });

        commandAdder add_stickmode(command_table, "stickmode", [this](const char* remainder) {
            std::string mode;
            advance_ws(remainder, mode);
            // TODO: this could be part of a G13::Constants class I think
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
        });

        commandAdder add_stickzone(command_table, "stickzone", [this](const char* remainder) {
            std::string operation, zonename;
            advance_ws(remainder, operation);
            advance_ws(remainder, zonename);
            if (operation == "add") {
                /* G13_StickZone* zone = */
                getStickRef().zone(zonename, true);
            }
            else {
                G13_StickZone* zone = getStickRef().zone(zonename);
                if (!zone) {
                    throw G13_CommandException("unknown stick zone");
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
                    zone->set_bounds(G13_ZoneBounds(x1, y1, x2, y2));
                }
                else if (operation == "del") {
                    getStickRef().RemoveZone(*zone);
                }
                else {
                    G13_ERR("unknown stickzone operation: <" << operation << ">");
                }
            }
        });

        commandAdder add_dump(command_table, "dump", [this](const char* remainder) {
            std::string target;
            advance_ws(remainder, target);
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
                G13_ERR("unknown dump target: <" << target << ">");
            }
        });

        commandAdder add_log_level(command_table, "log_level", [this](const char* remainder) {
            std::string level;
            advance_ws(remainder, level);
            SetLogLevel(level);
        });

        commandAdder add_refresh(command_table, "refresh", [this](const char* remainder) {
            getLCDRef().image_send();
        });

        commandAdder add_clear(command_table, "clear", [this](const char* remainder) {
            getLCDRef().image_clear();
            getLCDRef().image_send();
        });

        commandAdder add_delete(command_table, "delete", [this](const char* remainder) {
            std::string target;
            std::string glob;
            bool found = false;
            advance_ws(remainder, target);
            advance_ws(remainder, glob);
            const std::regex re(glob2regex(glob.c_str()));

            if (target == "profile") {
                for (auto& profile : FilteredProfileNames(re)) {
                    profiles.erase(profile);
                    G13_OUT("profile " << profile << " deleted");
                    found = true;
                }
            }
            else if (target == "key") {
                for (const auto& key : getCurrentProfileRef().FilteredKeyNames(re)) {
                    getCurrentProfileRef().FindKey(key)->set_action(nullptr);
                    G13_OUT("key " << key << " unbound");
                    found = true;
                }
            }
            else if (target == "zone") {
                for (auto& zone : getStickRef().FilteredZoneNames(re)) {
                    getStickRef().RemoveZone(*getStickRef().zone(zone));
                    G13_OUT("stickzone " << zone << " unbound");
                    found = true;
                }
            }
            else {
                G13_ERR("unknown delete target: <" << target << ">");
                found = true;
            }
            if (!found) {
                G13_OUT("no " << target << " name matches <" << glob << ">");
            }
        });

        commandAdder add_load(command_table, "load", [this](const char* remainder) {
            std::string filename;
            advance_ws(remainder, filename);
            ReadCommandsFromFile(filename, std::string(1 + files_currently_loading.size(), '>').c_str());
        });
    }

    void G13_Device::Command(char const* str, const char* info) {
        const char* remainder = str;

        try {
            std::string cmd;
            advance_ws(remainder, cmd);

            if (!cmd.empty()) {
                const auto i = command_table.find(cmd);
                if (info)
                    G13_OUT(info << ": " << ltrim(str));
                if (i == command_table.end()) {
                    G13_ERR("unknown command : " << cmd);
                }
                else {
                    const COMMAND_FUNCTION f = i->second;
                    f(remainder);
                }
            }
        }
        catch (const std::exception& ex) {
            G13_ERR("command failed : " << ex.what());
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
        input_pipe_name = MakePipeName(this, true);
        input_pipe_fid = G13CreateFifo(input_pipe_name.c_str(), S_IRGRP | S_IROTH);

        if (input_pipe_fid == -1) {
            G13_ERR("failed opening input pipe " << input_pipe_name);
        }

        output_pipe_name = MakePipeName(this, false);
        output_pipe_fid = G13CreateFifo(output_pipe_name.c_str(), S_IWGRP | S_IWOTH);

        if (output_pipe_fid == -1) {
            G13_ERR("failed opening output pipe " << output_pipe_name);
        }
    }

    void G13_Device::Cleanup() const {
        SetKeyColor(0, 0, 0);
        remove(input_pipe_name.c_str());
        remove(output_pipe_name.c_str());
        ioctl(uinput_fid, UI_DEV_DESTROY);
        close(uinput_fid);
        libusb_release_interface(usb_handle, 0);
        libusb_close(usb_handle);
    }

} // namespace G13
