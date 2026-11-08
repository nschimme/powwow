if(NOT WIN32)
    add_executable(powwow main.c tty.c edit.c)
    target_link_libraries(powwow PRIVATE powwow_core)

    find_package(Curses QUIET)
    if(CURSES_FOUND)
        target_include_directories(powwow PRIVATE ${CURSES_INCLUDE_DIRS})
        target_link_libraries(powwow PRIVATE ${CURSES_LIBRARIES})
    else()
        target_link_libraries(powwow PRIVATE ncurses)
    endif()

    add_executable(powwow-muc powwow-muc.c)
    add_executable(powwow-movieplay powwow-movieplay.c)

    target_include_directories(powwow-muc PRIVATE "${CMAKE_SOURCE_DIR}/libpowwow")
    target_include_directories(powwow-movieplay PRIVATE "${CMAKE_SOURCE_DIR}/libpowwow")

    target_link_libraries(powwow-muc PRIVATE powwow_platform_definitions)
    target_link_libraries(powwow-movieplay PRIVATE powwow_platform_definitions)

    if(CURSES_FOUND)
        target_include_directories(powwow-muc PRIVATE ${CURSES_INCLUDE_DIRS})
        target_link_libraries(powwow-muc PRIVATE ${CURSES_LIBRARIES})
        target_include_directories(powwow-movieplay PRIVATE ${CURSES_INCLUDE_DIRS})
        target_link_libraries(powwow-movieplay PRIVATE ${CURSES_LIBRARIES})
    else()
        target_link_libraries(powwow-muc PRIVATE ncurses)
        target_link_libraries(powwow-movieplay PRIVATE ncurses)
    endif()

    install(TARGETS powwow powwow-muc powwow-movieplay DESTINATION bin)

    add_subdirectory(tests)
endif()
