#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    printf("Testing TUI frontend CLI flags and execution...\n");

    /* 1. Test --version */
    int r1 = system("./powwow -v > /dev/null 2>&1");
    assert(r1 == 0);

    /* 2. Test -vv (detailed version) */
    int r2 = system("./powwow -vv > /dev/null 2>&1");
    assert(r2 == 0);

    /* 3. Test --help */
    int r3 = system("./powwow --help > /dev/null 2>&1");
    assert(r3 == 0);

    /* 4. Test invalid options fail cleanly */
    int r4 = system("./powwow --invalid-option-xyz > /dev/null 2>&1");
    assert(r4 != 0);

    printf("test_tui PASSED!\n");
    return 0;
}
