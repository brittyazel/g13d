#
# Project: g13d
# Language: C++
# License: MIT
# Author: Britt W. Yazel <bwyazel@gmail.com>
#
# This project is a daemon for managing and interacting with the Logitech
# G13 gameboard. It includes functionalities for handling device input,
# managing profiles, and displaying information on the G13's LCD screen.
#

# Project Configuration
project(
    'g13d',
    'cpp',
    version : '0.1.0',
    default_options : ['cpp_std=c++23'],
)

# Project Arguments
add_project_arguments('-DCONTROL_DIR="' + get_option('control_dir') + '"', language : 'cpp')
add_project_arguments('-DPROJECT_VERSION="' + meson.project_version() + '"', language : 'cpp')

# Sources
project_src_files = files(
    'src/main.cpp',
    'src/action_keys.cpp',
    'src/action_pipeout.cpp',
    'src/stickzone.cpp',
    'src/action_command.cpp',
    'src/device.cpp',
    'src/font.cpp',
    'src/font_char.cpp',
    'src/lifecycle.cpp',
    'src/key_state.cpp',
    'src/key_tables.cpp',
    'src/key.cpp',
    'src/lcd.cpp',
    'src/log.cpp',
    'src/profile.cpp',
    'src/stick.cpp',
    'src/utilities/utilities.cpp',
    'src/utilities/escaped_string_formatter.cpp',
)

# Dependencies
project_dependencies = [
    dependency('libusb-1.0'),
    dependency('log4cpp'),
    dependency('libevdev'),
    dependency('libsystemd'),
]

# Executables
g13d = executable(
    'g13d',
    project_src_files,
    include_directories : ['include'],
    dependencies : project_dependencies,
    install : true
)

# Installation
install_subdir('host', install_dir : '/usr/lib', strip_directory : true)