#define WBUILD_IMPL
#include "wbuild.h"

int main(int argc, char** argv) {
    add_target("wbuild");
    target_depends("wbuild", "wbuild.c", "wbuild.h");
    target_commands(
        "wbuild",
        "cc wbuild.c -o wbuild -g -fsanitize=address,leak"
    );
    return 0;
}
