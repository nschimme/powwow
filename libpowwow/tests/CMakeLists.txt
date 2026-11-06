add_executable(test_charset test_charset.c ../charset.c ../width.c)
target_link_libraries(test_charset PRIVATE powwow_platform_definitions)
target_include_directories(test_charset PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_test(NAME libpowwow_test_charset COMMAND test_charset)

add_executable(test_sgr test_sgr.c ../sgr.c)
target_link_libraries(test_sgr PRIVATE powwow_platform_definitions)
target_include_directories(test_sgr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_test(NAME libpowwow_test_sgr COMMAND test_sgr)

add_executable(test_libpowwow test_libpowwow.c)
target_link_libraries(test_libpowwow PRIVATE powwow_core)
add_test(NAME libpowwow_test_main COMMAND test_libpowwow)

add_executable(test_session test_session.c)
target_link_libraries(test_session PRIVATE powwow_core)
add_test(NAME libpowwow_test_session COMMAND test_session)

add_executable(test_alias test_alias.c)
target_link_libraries(test_alias PRIVATE powwow_core)
add_test(NAME libpowwow_test_alias COMMAND test_alias)

add_executable(test_action test_action.c)
target_link_libraries(test_action PRIVATE powwow_core)
add_test(NAME libpowwow_test_action COMMAND test_action)

add_executable(test_eval test_eval.c)
target_link_libraries(test_eval PRIVATE powwow_core)
add_test(NAME libpowwow_test_eval COMMAND test_eval)

add_executable(test_bind_mark_history test_bind_mark_history.c)
target_link_libraries(test_bind_mark_history PRIVATE powwow_core)
add_test(NAME libpowwow_test_bind_mark_history COMMAND test_bind_mark_history)
