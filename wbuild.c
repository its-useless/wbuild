#define WBUILD_IMPL
#include "wbuild.h"

int main(int argc, char** argv) {
    add_target("wbuild.c");
    target_depends("wbuild.c", "wbuild.h");
    target_commands("wbuild.c", "touch wbuild.c");

    add_target("wbuild.o");
    target_depends("wbuild.o", "wbuild.c");
    target_commands(
        "wbuild.o",
        "cc wbuild.c -o wbuild.o -c -g -fsanitize=address,leak"
    );

    add_target("wbuild");
    target_depends("wbuild", "wbuild.o");
    target_commands(
        "wbuild",
        "cc wbuild.o -o wbuild -g -fsanitize=address,leak"
    );
    return 0;
}
