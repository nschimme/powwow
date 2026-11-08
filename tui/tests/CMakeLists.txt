add_executable(test_tui test_tui.c)
target_compile_definitions(test_tui PRIVATE POWWOW_BIN="$<TARGET_FILE:powwow>")
add_test(NAME tui_test_cli COMMAND test_tui)
