//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef DEVICE_HPP
#define DEVICE_HPP


#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <map>
#include <regex>
#include <vector>

#include "Font.hpp"
#include "Screen.hpp"
#include "Profile.hpp"
#include "Stick.hpp"


namespace G13 {
    class Action; // Forward declaration

    constexpr size_t NUM_KEYS = 40;

    inline void IGUR(...) {}

    class Device {
    public:
        typedef std::function<void(const char*)> COMMAND_FUNCTION;
        typedef std::map<std::string, COMMAND_FUNCTION> CommandFunctionTable;

        bool connected;

        Device(libusb_device* usb_device, libusb_context* usb_context, libusb_device_handle* usb_handle,
                   int device_index);
        ~Device();

        void Cleanup();
        void RegisterContext(libusb_context* new_usb_context);

        Screen& getScreenRef();
        Stick& getStickRef();

        std::shared_ptr<Font> SwitchToFont(const std::string& name);
        void SwitchToProfile(const std::string& name);

        [[nodiscard]] std::vector<std::string> FilteredProfileNames(const std::regex& pattern) const;

        void Dump(std::ostream& o, int detail = 0);
        void Command(const char* str, const char* info = nullptr);
        void ReadCommandsFromPipe();
        int ReadDeviceInputs();
        void ReadCommandsFromFile(const std::string& filename, const char* info = nullptr);
        static int G13CreateUinput();
        static int G13CreateFifo(const char* fifo_name, mode_t umask);

        std::shared_ptr<Action> MakeAction(const std::string& action);
        void SetKeyColor(int red, int green, int blue) const;
        void SetModeLeds(int leds) const;
        void SendEvent(int type, int code, int val);
        void OutputPipeWrite(const std::string& out) const;
        bool updateKeyState(int key, bool state);
        static std::string DescribeLibusbErrorCode(int code);

        [[nodiscard]] int getDeviceIndex() const;
        [[nodiscard]] libusb_device_handle* getHandlePtr() const;
        [[nodiscard]] libusb_device* getDevicePtr() const;
        [[nodiscard]] Font& getCurrentFontRef() const;
        [[nodiscard]] Profile& getCurrentProfileRef() const;
        static Device* GetG13DeviceHandle(const libusb_device* dev);

    protected:
        void InitFonts();
        void InitScreen();
        void InitCommands();

    private:
        static bool IsDataAvailable(int fd);
        void ProcessBuffer(char* buffer, int buffer_end, int read_result);
        [[nodiscard]] std::string NormalizeFilePath(const std::string& filename) const;
        void MakePipeNames();

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

        std::map<std::string, std::shared_ptr<Font>> fonts;
        std::shared_ptr<Font> current_font;
        std::map<std::string, std::shared_ptr<Profile>> profiles;
        std::shared_ptr<Profile> current_profile;
        std::vector<std::string> files_currently_loading;

        Screen screen;
        Stick stick;
        bool keys[NUM_KEYS]{};

        libusb_device_handle* usb_handle;
        libusb_device* usb_device;
    };
}

#endif
