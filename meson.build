project('g13d', 'cpp',
        version: '0.1.0',
        default_options: ['cpp_std=c++23']
)

# Define CONTROL_DIR
add_project_arguments('-DCONTROL_DIR="/run/g13d"', language : 'cpp')

# Sources
sources = files(
    'src/g13_main.cpp',
    'src/g13_action_keys.cpp',
    'src/g13_action_pipeout.cpp',
    'src/g13_stickzone.cpp',
    'src/g13_action_command.cpp',
    'src/g13_device.cpp',
    'src/g13_font.cpp',
    'src/g13_font_char.cpp',
    'src/g13_hotplug.cpp',
    'src/g13_state_key.cpp',
    'src/g13_key_tables.cpp',
    'src/g13_key.cpp',
    'src/g13_lcd.cpp',
    'src/g13_log.cpp',
    'src/g13_profile.cpp',
    'src/g13_stick.cpp',
    'src/helper.cpp'
)

# Include directories
inc = include_directories('include', 'src')

# Dependencies
usb = dependency('libusb-1.0')
log4cpp = dependency('log4cpp', required: false)
libevdev = dependency('libevdev', required: false)

# Executables
g13d = executable('g13d', sources, include_directories: inc, dependencies: [usb, log4cpp, libevdev])
pbm2lpbm = executable('pbm2lpbm', 'src/utils/pbm2lpbm.cpp', include_directories: inc)

# Installation
install_subdir('host', install_dir: '.')