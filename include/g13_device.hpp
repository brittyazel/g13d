//
// Created by khampf on 07-05-2020.
//

#ifndef G13_DEVICE_HPP
#define G13_DEVICE_HPP

#include <functional>
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <map>
#include <memory>
#include <regex>
#include <vector>

#include "g13_lcd.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"
#include "g13_font.hpp"

namespace G13 {
    class G13_Profile;
    class G13_Font;

    typedef std::shared_ptr<G13_Profile> ProfilePtr;

    constexpr size_t G13_NUM_KEYS = 40;

    int G13CreateUinput(G13_Device* g13);
    int G13CreateFifo(const char* fifo_name, mode_t umask);

    class G13_Device {
    public:
        typedef std::function<void(const char*)> COMMAND_FUNCTION;
        typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

        G13_Device(libusb_device* usb_device, libusb_context* usb_context, libusb_device_handle* usb_handle,
                   int device_index);
        ~G13_Device();

        G13_LCD& getLCDRef();
        G13_Stick& getStickRef();

        std::shared_ptr<G13_Font> SwitchToFont(const std::string& name);
        void SwitchToProfile(const std::string& name);

        [[nodiscard]] std::vector<std::string> FilteredProfileNames(const std::regex& pattern) const;
        ProfilePtr Profile(const std::string& name);

        void Dump(std::ostream& o, int detail = 0);
        void Command(const char* str, const char* info = nullptr);
        void ReadCommandsFromPipe();
        void ReadCommandsFromFile(const std::string& filename, const char* info = nullptr);
        void ReadConfigFile(const std::string& filename);
        int ReadKeypresses();
        void parse_joystick(const unsigned char* buf);

        std::shared_ptr<G13_Action> MakeAction(const std::string& action);
        void SetKeyColor(int red, int green, int blue) const;
        void SetModeLeds(int leds) const;
        void SendEvent(int type, int code, int val);
        void OutputPipeWrite(const std::string& out) const;
        void LcdWrite(const unsigned char* data, size_t size) const;
        bool updateKeyState(int key, bool state);

        void Cleanup() const;
        void RegisterContext(libusb_context* new_usb_context);
        void LcdWriteFile(const std::string& filename) const;
        static std::string DescribeLibusbErrorCode(int code);

        [[nodiscard]] int getDeviceIndex() const;
        [[nodiscard]] libusb_device_handle* getHandlePtr() const;
        [[nodiscard]] libusb_device* getDevicePtr() const;
        [[nodiscard]] G13_Font& getCurrentFontRef() const;
        [[nodiscard]] G13_Profile& getCurrentProfileRef() const;

    protected:
        void InitFonts();
        void LcdInit() const;
        void InitCommands();

        CommandFunctionTable command_table;
        input_event device_event{};

        int device_index;
        libusb_context* usb_context;
        int uinput_fid;
        int input_pipe_fid{};
        std::string input_pipe_name;
        std::string input_pipe_fifo;
        int output_pipe_fid{};
        std::string output_pipe_name;

        std::map<std::string, std::shared_ptr<G13_Font>> fonts;
        std::shared_ptr<G13_Font> current_font;
        std::map<std::string, ProfilePtr> profiles;
        ProfilePtr current_profile;
        std::vector<std::string> files_currently_loading;

        G13_LCD lcd;
        G13_Stick stick;
        bool keys[G13_NUM_KEYS]{};

    private:
        libusb_device_handle* usb_handle;
        libusb_device* usb_device;
    };
} // namespace G13

#endif // G13_DEVICE_HPP
