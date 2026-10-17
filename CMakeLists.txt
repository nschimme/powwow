cmake_minimum_required(VERSION 3.16)
project(powwow VERSION 1.3.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(ENABLE_TLS "Enable TLS via GnuTLS" ON)
option(BUILD_GUI "Build Qt6 GUI Launcher and Terminal" ON)

# Definitions and config header simulation for C core
add_definitions(-D_XOPEN_SOURCE=700)
add_definitions(-DPOWWOW_DIR="${CMAKE_INSTALL_PREFIX}/share/powwow")
add_definitions(-DPLUGIN_DIR="${CMAKE_INSTALL_PREFIX}/lib/powwow")
add_definitions(-DVERSION="${PROJECT_VERSION}")

if(UNIX)
    add_definitions(-DHAVE_GETOPT_LONG=1)
    add_definitions(-DUSE_REGEXP=1)
    add_definitions(-DUSE_SHELL=1)
    add_definitions(-DHAVE_STDIO_H=1)
    add_definitions(-DHAVE_STDLIB_H=1)
    add_definitions(-DHAVE_STRING_H=1)
    add_definitions(-DHAVE_UNISTD_H=1)
    add_definitions(-DHAVE_SYS_STAT_H=1)
    add_definitions(-DHAVE_SYS_TYPES_H=1)
endif()

if(ENABLE_TLS AND UNIX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(GNUTLS gnutls)
    endif()
    if(GNUTLS_FOUND)
        add_definitions(-DUSE_TLS=1 -DHAVE_GNUTLS=1)
    else()
        find_library(GNUTLS_LIB gnutls)
        if(GNUTLS_LIB)
            add_definitions(-DUSE_TLS=1 -DHAVE_GNUTLS=1)
        endif()
    endif()
endif()

# C source files for powwow CLI (Unix/POSIX)
if(NOT WIN32)
    find_package(Curses QUIET)
    set(POWWOW_C_SOURCES
        src/beam.c
        src/cmd.c
        src/log.c
        src/edit.c
        src/cmd2.c
        src/eval.c
        src/utils.c
        src/main.c
        src/tcp.c
        src/list.c
        src/map.c
        src/tty.c
        src/ptr.c
        src/tls.c
        src/charset.c
        src/width.c
        src/sgr.c
    )

    add_executable(powwow ${POWWOW_C_SOURCES})
    if(CURSES_FOUND)
        target_link_libraries(powwow PRIVATE ${CURSES_LIBRARIES})
    else()
        target_link_libraries(powwow PRIVATE ncurses)
    endif()

    if(GNUTLS_FOUND)
        target_include_directories(powwow PRIVATE ${GNUTLS_INCLUDE_DIRS})
        target_link_libraries(powwow PRIVATE ${GNUTLS_LIBRARIES})
    elseif(GNUTLS_LIB)
        target_link_libraries(powwow PRIVATE ${GNUTLS_LIB})
    endif()

    if(CMAKE_DL_LIBS)
        target_link_libraries(powwow PRIVATE ${CMAKE_DL_LIBS})
        add_definitions(-DHAVE_LIBDL=1)
    endif()

    # powwow utilities
    add_executable(powwow-muc src/powwow-muc.c)
    add_executable(powwow-movieplay src/powwow-movieplay.c)

    if(CURSES_FOUND)
        target_link_libraries(powwow-muc PRIVATE ${CURSES_LIBRARIES})
        target_link_libraries(powwow-movieplay PRIVATE ${CURSES_LIBRARIES})
    else()
        target_link_libraries(powwow-muc PRIVATE ncurses)
        target_link_libraries(powwow-movieplay PRIVATE ncurses)
    endif()

    # Tests
    enable_testing()
    add_executable(test_charset src/test_charset.c src/charset.c src/width.c)
    add_test(NAME test_charset COMMAND test_charset)

    add_executable(test_sgr src/test_sgr.c src/sgr.c)
    add_test(NAME test_sgr COMMAND test_sgr)
endif()

# Qt6 GUI Application
if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Widgets Network QUIET)
    if(Qt6_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)

        set(GUI_SOURCES
            gui/main.cpp
            gui/mainwindow.cpp
            gui/mainwindow.h
            gui/terminalwidget.cpp
            gui/terminalwidget.h
            gui/profilemanager.cpp
            gui/profilemanager.h
        )

        add_executable(powwow-gui ${GUI_SOURCES})
        target_link_libraries(powwow-gui PRIVATE
            Qt6::Core
            Qt6::Widgets
            Qt6::Network
        )
    else()
        message(STATUS "Qt6 not found. Skipping powwow-gui build.")
    endif()
endif()
