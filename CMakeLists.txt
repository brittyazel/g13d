cmake_minimum_required(VERSION 3.16)
project(g13d)

set(CMAKE_CXX_STANDARD 23)

option(DEBUG "Compile for debugging" OFF)
if(DEBUG)
        add_compile_options(-g)
        add_link_options(-g)
endif()

set(CONTROL_DIR "/run/g13d" CACHE STRING "Absolute directory of default pipe files (/run/g13d/)")
add_definitions(-DCONTROL_DIR="${CONTROL_DIR}")

add_executable(g13d
        include/version.hpp
        include/logo.hpp
        include/g13_main.hpp
        src/g13_main.cpp
        include/g13_action.hpp
        src/g13_action.cpp
        include/g13_device.hpp
        src/g13_device.cpp
        include/g13_fonts.hpp
        src/g13_fonts.cpp
        include/g13_hotplug.hpp
        src/g13_hotplug.cpp
        include/g13_keys.hpp
        src/g13_keys.cpp
        include/g13_lcd.hpp
        src/g13_lcd.cpp
        include/g13_log.hpp
        src/g13_log.cpp
        include/g13_manager.hpp
        src/g13_manager.cpp
        include/g13_profile.hpp
        src/g13_profile.cpp
        include/g13_stick.hpp
        src/g13_stick.cpp
        include/helper.hpp
        src/helper.cpp
)

add_executable(runtests
        include/version.hpp
        include/logo.hpp
        include/g13_main.hpp
        src/g13_main_test.cpp
        include/g13_action.hpp
        src/g13_action.cpp
        include/g13_device.hpp
        src/g13_device.cpp
        include/g13_fonts.hpp
        src/g13_fonts.cpp
        include/g13_hotplug.hpp
        src/g13_hotplug.cpp
        include/g13_keys.hpp
        src/g13_keys.cpp
        include/g13_lcd.hpp
        src/g13_lcd.cpp
        include/g13_log.hpp
        src/g13_log.cpp
        include/g13_manager.hpp
        src/g13_manager.cpp
        include/g13_profile.hpp
        src/g13_profile.cpp
        include/g13_stick.hpp
        src/g13_stick.cpp
        include/helper.hpp
        src/helper.cpp
)

add_executable(pbm2lpbm src/utils/pbm2lpbm.cpp)

target_include_directories(g13d PRIVATE include src)
target_include_directories(runtests PRIVATE include src)

target_link_libraries(g13d usb-1.0 log4cpp evdev)
target_link_libraries(runtests usb-1.0 log4cpp evdev gtest gmock)

install(TARGETS g13d pbm2lpbm
        RUNTIME DESTINATION usr/bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/host/
        DESTINATION .
)