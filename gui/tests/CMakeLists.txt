add_executable(test_gui
    test_gui.cpp
    ../profilemanager.cpp
    ../profilemanager.h
    ../mudconfigdialog.cpp
    ../mudconfigdialog.h
    ../terminalwidget.cpp
    ../terminalwidget.h
    ../resources.qrc
)
target_include_directories(test_gui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_SOURCE_DIR}/include"
)
target_link_libraries(test_gui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)
if(TARGET powwow_core)
    target_link_libraries(test_gui PRIVATE powwow_core)
endif()
add_test(NAME gui_test_smoke COMMAND test_gui)
