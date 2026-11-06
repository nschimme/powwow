if(BUILD_GUI)
    find_package(Qt6 COMPONENTS Core Widgets Network QUIET)
    if(Qt6_FOUND)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)

        if(MSVC AND NOT BUILD_SHARED_LIBS)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()

        set(GUI_SOURCES
            main.cpp
            mainwindow.cpp
            mainwindow.h
            terminalwidget.cpp
            terminalwidget.h
            profilemanager.cpp
            profilemanager.h
            mudconfigdialog.cpp
            mudconfigdialog.h
            resources.qrc
        )

        if(WIN32)
            list(APPEND GUI_SOURCES powtty.rc)
        endif()

        if(APPLE)
            set(MACOS_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns")
            set_source_files_properties(${MACOS_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
            add_executable(powtty MACOSX_BUNDLE ${GUI_SOURCES} ${MACOS_ICON_FILE})
            set_target_properties(powtty PROPERTIES
                MACOSX_BUNDLE_BUNDLE_NAME "PowTTY"
                MACOSX_BUNDLE_GUI_IDENTIFIER "org.mume.powtty"
                MACOSX_BUNDLE_ICON_FILE "icon.icns"
            )
        else()
            add_executable(powtty ${GUI_SOURCES})
        endif()

        target_link_libraries(powtty PRIVATE
            powwow_core
            Qt6::Core
            Qt6::Widgets
            Qt6::Network
        )

        install(TARGETS powtty DESTINATION bin)

        if(WIN32)
            find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${Qt6_DIR}/../../../bin)
            if(WINDEPLOYQT_EXECUTABLE)
                add_custom_command(TARGET powtty POST_BUILD
                    COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-compiler-runtime $<TARGET_FILE:powtty>
                    COMMENT "Running windeployqt..."
                )
            endif()
        elseif(APPLE)
            find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS ${Qt6_DIR}/../../../bin)
            if(MACDEPLOYQT_EXECUTABLE)
                add_custom_command(TARGET powtty POST_BUILD
                    COMMAND ${MACDEPLOYQT_EXECUTABLE} $<TARGET_BUNDLE_DIR:powtty> -always-overwrite
                    COMMENT "Running macdeployqt..."
                )
            endif()
        endif()

        add_subdirectory(tests)
    else()
        message(STATUS "Qt6 not found. Skipping powtty build.")
    endif()
endif()
