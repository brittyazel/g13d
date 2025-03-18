//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef G13_DEVICE_HPP
#define G13_DEVICE_HPP


#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>

#include <g13_action.hpp>
#include "g13_lcd.hpp"
#include "g13_profile.hpp"
#include "g13_stick.hpp"


namespace G13 {
    class G13_Profile;
    class G13_Font;

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
        std::shared_ptr<G13_Profile> Profile(const std::string& name);

        void Dump(std::ostream& o, int detail = 0);
        void Command(const char* str, const char* info = nullptr);
        void ReadCommandsFromPipe();
        int ReadKeypresses();
        void ReadCommandsFromFile(const std::string& filename, const char* info = nullptr);

        std::shared_ptr<G13_Action> MakeAction(const std::string& action);
        void SetKeyColor(int red, int green, int blue) const;
        void SetModeLeds(int leds) const;
        void SendEvent(int type, int code, int val);
        void OutputPipeWrite(const std::string& out) const;
        void LcdWrite(const unsigned char* data, size_t size) const;
        bool updateKeyState(int key, bool state);

        void Cleanup();
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

    private:
        static bool IsDataAvailable(int fd);
        void ProcessBuffer(char* buffer, int buffer_end, int read_result);
        [[nodiscard]] std::string NormalizeFilePath(const std::string& filename) const;
        void parse_joystick(const unsigned char* buf);

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
        std::map<std::string, std::shared_ptr<G13_Profile>> profiles;
        std::shared_ptr<G13_Profile> current_profile;
        std::vector<std::string> files_currently_loading;

        G13_LCD lcd;
        G13_Stick stick;
        bool keys[G13_NUM_KEYS]{};

        libusb_device_handle* usb_handle;
        libusb_device* usb_device;
    };
}

#endif // G13_DEVICE_HPP