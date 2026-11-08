#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef POWWOW_BIN
#  define POWWOW_BIN "./powwow"
#endif

int main(void) {
    char buf[512];

    /* 1. Test -v */
    snprintf(buf, sizeof(buf), "%s -v > /dev/null 2>&1", POWWOW_BIN);
    assert(system(buf) == 0);

    /* 2. Test -vv (detailed version) */
    snprintf(buf, sizeof(buf), "%s -vv > /dev/null 2>&1", POWWOW_BIN);
    assert(system(buf) == 0);

    /* 3. Test --help */
    snprintf(buf, sizeof(buf), "%s --help > /dev/null 2>&1", POWWOW_BIN);
    assert(system(buf) == 0);

    /* 4. Test invalid options fail */
    snprintf(buf, sizeof(buf), "%s --invalid-option-xyz > /dev/null 2>&1", POWWOW_BIN);
    assert(system(buf) != 0);

    printf("test_tui PASSED!\n");
    return 0;
}
