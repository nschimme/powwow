if(NOT WIN32)
    set(LIBPOWWOW_SOURCES
        powwow.c
        core_engine.c
        beam.c
        cmd.c
        cmd2.c
        eval.c
        list.c
        log.c
        map.c
        ptr.c
        sgr.c
        charset.c
        width.c
        utils.c
        tcp.c
        tls.c
        ${CMAKE_SOURCE_DIR}/tui/tty.c
        ${CMAKE_SOURCE_DIR}/tui/edit.c
    )

    add_library(powwow_core STATIC ${LIBPOWWOW_SOURCES})
    target_link_libraries(powwow_core PUBLIC powwow_platform_definitions)
    target_include_directories(powwow_core PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/tui"
    )

    if(TARGET PkgConfig::GNUTLS)
        target_link_libraries(powwow_core PUBLIC PkgConfig::GNUTLS)
        target_compile_definitions(powwow_core PUBLIC USE_TLS=1 HAVE_GNUTLS=1)
    endif()

    find_package(Curses QUIET)
    if(CURSES_FOUND)
        target_include_directories(powwow_core PRIVATE ${CURSES_INCLUDE_DIRS})
        target_link_libraries(powwow_core PUBLIC ${CURSES_LIBRARIES})
    else()
        target_link_libraries(powwow_core PUBLIC ncurses)
    endif()

    if(CMAKE_DL_LIBS)
        target_link_libraries(powwow_core PUBLIC ${CMAKE_DL_LIBS})
        target_compile_definitions(powwow_core PUBLIC HAVE_LIBDL=1)
    endif()

    # Library-specific Unit Tests
    add_subdirectory(tests)
endif()
