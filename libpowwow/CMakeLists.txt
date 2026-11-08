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
    ui_hook.c
)

add_library(powwow_core STATIC ${LIBPOWWOW_SOURCES})
target_link_libraries(powwow_core PUBLIC powwow_platform_definitions)
target_include_directories(powwow_core PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(TARGET PkgConfig::GNUTLS)
    target_link_libraries(powwow_core PUBLIC PkgConfig::GNUTLS)
    target_compile_definitions(powwow_core PUBLIC USE_TLS=1 HAVE_GNUTLS=1)
endif()

if(WIN32)
    target_link_libraries(powwow_core PUBLIC ws2_32)
endif()

if(CMAKE_DL_LIBS)
    target_link_libraries(powwow_core PUBLIC ${CMAKE_DL_LIBS})
    target_compile_definitions(powwow_core PUBLIC HAVE_LIBDL=1)
endif()

# Library-specific Unit Tests
add_subdirectory(tests)
