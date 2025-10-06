/*
 * its-useless/wbuild
 * aceinetx 2021-2025
 */
#ifndef WBUILD_H
#define WBUILD_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/// ARENA
typedef struct {
    void* data;
    size_t size;
    void* next;
} Arena;

Arena arena_new(size_t size);
void* arena_alloc(Arena* arena, size_t size);
void arena_free(Arena* arena);

void add_target(char* output);
void _target_depends(char* target, const char* const* depends);
void _target_commands(char* target, const char* const* commands);

#define target_depends(target, ...) \
    _target_depends(target, (const char* const[]) {__VA_ARGS__, NULL});
#define target_commands(target, ...) \
    _target_commands(target, (const char* const[]) {__VA_ARGS__, NULL});

#ifdef WBUILD_IMPL
/// ARENA
Arena arena_new(size_t size) {
    Arena arena;
    arena.next = arena.data = malloc(size);
    arena.size = size;
    return arena;
}

void* arena_alloc(Arena* arena, size_t size) {
    if (arena->size - (arena->next - arena->data) < size) {
        puts("arena_alloc: out of arena memory");
        return NULL;
    }
    void* p = arena->next;
    arena->next += size;
    return p;
}

void arena_free(Arena* arena) {
    free(arena->data);
}

static Arena global_arena;
    #define aalloc(size) arena_alloc(&global_arena, size)

/// UTIL
static void quit(int code) {
    arena_free(&global_arena);
    exit(code);
}

/// FILESYSTEM
static bool fexists(const char* name) {
    return (access(name, F_OK) != -1);
}

static uint64_t get_modified_date(const char* name) {
    struct stat st;
    if (stat(name, &st) != 0)
        return 0;
    int64_t sec = (int64_t)st.st_mtim.tv_sec;
    int64_t nsec = (int64_t)st.st_mtim.tv_nsec;
    return sec * 1000 + nsec / 1000000;
}

/// TARGET
typedef struct {
    char* output;

    char** depends;
    size_t depends_len;

    char** commands;
    size_t commands_len;

    bool is_built;
} Target;

typedef struct TargetNode {
    Target* target;
    struct TargetNode* next;
} TargetNode;

static TargetNode targets = {0};
static size_t built_targets = 0;

size_t targets_len() {
    size_t len = 0;

    TargetNode* node = &targets;
    for (;;) {
        if (!node->target || !node)
            break;

        len++;

        node = node->next;
    }

    return len;
}

static int get_percentage() {
    size_t len = targets_len();
    if (len == built_targets)
        return 100;

    int percent = 100 / len * (built_targets);

    if (percent > 100)
        percent = 100;
    return percent;
}

bool target_needs_building(Target* target) {
    bool modified = false;
    if (target->is_built)
        return false;

    for (size_t i = 0; i < target->depends_len; i++) {
        char* dependency = target->depends[i];

        TargetNode* node = &targets;
        for (;;) {
            if (!node->target || !node)
                break;

            if (strcmp(node->target->output, dependency) != 0) {
                node = node->next;
                continue;
            }

            if (target_needs_building(node->target)) {
                return true;
            }

            node = node->next;
        }
    }

    uint64_t output_date = get_modified_date(target->output);
    for (size_t i = 0; i < target->depends_len; i++) {
        char* dependency = target->depends[i];

        if (get_modified_date(dependency) >= output_date) {
            modified = true;
            break;
        }
    }

    return modified;
}

bool target_build(Target* target) {
    if (!target_needs_building(target))
        return true;

    built_targets++;
    int percent = get_percentage();

    printf("[%3d%%] building %s\n", percent, target->output);

    for (size_t i = 0; i < target->depends_len; i++) {
        char* dependency = target->depends[i];

        TargetNode* node = &targets;
        for (;;) {
            if (!node->target || !node)
                break;

            if (strcmp(node->target->output, dependency) == 0) {
                if (!target_build(node->target))
                    return false;
                break;
            }
            node = node->next;
        }

        if (!fexists(dependency))
            return false;
    }

    percent = get_percentage();
    for (size_t i = 0; i < target->commands_len; i++) {
        char* command = target->commands[i];

        printf("[%3d%%] running build cmd for %s\n", percent, target->output);

        system(command);
    }
    target->is_built = true;

    return true;
}

void add_target(char* output) {
    Target* target = aalloc(sizeof(Target));
    target->is_built = false;

    size_t output_sz = strlen(output) + 1;
    target->output = aalloc(output_sz);
    memcpy(target->output, output, output_sz);

    TargetNode* node = &targets;
    for (;;) {
        if (!node->next) {
            TargetNode* new_node = aalloc(sizeof(TargetNode));
            new_node->next = NULL;
            new_node->target = NULL;
            node->next = new_node;
        }
        if (!node->target) {
            node->target = target;
            break;
        }
        node = node->next;
    }
}

static Target* find_target(char* target) {
    TargetNode* node = &targets;
    Target* tgt = NULL;
    for (;;) {
        if (!node->target || !node)
            break;
        if (strcmp(node->target->output, target) == 0) {
            tgt = node->target;
            break;
        }
        node = node->next;
    }
    if (!tgt) {
        printf("[...] find_target failed: no target %s\n", target);
        quit(1);
    }
    return tgt;
}

void _target_depends(char* target, const char* const* depends) {
    Target* tgt = find_target(target);

    size_t count;
    for (count = 0; depends[count]; count++)
        ;

    char** array = aalloc(count * sizeof(char*));
    for (size_t i = 0; i < count; i++) {
        size_t sz = strlen(depends[i]) + 1;
        char* str = aalloc(sz);
        memcpy(str, depends[i], sz);
        array[i] = str;
    }

    tgt->depends = array;
    tgt->depends_len = count;
}

void _target_commands(char* target, const char* const* commands) {
    Target* tgt = find_target(target);

    size_t count;
    for (count = 0; commands[count]; count++)
        ;

    char** array = aalloc(count * sizeof(char*));
    for (size_t i = 0; i < count; i++) {
        size_t sz = strlen(commands[i]) + 1;
        char* str = aalloc(sz);
        memcpy(str, commands[i], sz);
        array[i] = str;
    }

    tgt->commands = array;
    tgt->commands_len = count;
}

int wbuild_main(int, char**);

int main(int argc, char** argv) {
    int res;

    global_arena = arena_new(0x1000);

    puts("[    ] useless/wbuild aceinetx 2021-2025");
    if (argc < 2) {
        puts("[    ] no action provided, actions: build | clean");
        quit(1);
    }

    if ((res = wbuild_main(argc, argv)) != 0) {
        quit(res);
    }
    if (strcmp(argv[1], "build") == 0) {
        TargetNode* node = &targets;
        for (;;) {
            if (!node->target || !node)
                break;

            if (!target_build(node->target)) {
                printf("[!  !] build fail for %s\n", node->target->output);
                quit(1);
            }
            node = node->next;
        }
    } else if (strcmp(argv[1], "clean") == 0) {
        TargetNode* node = &targets;
        for (;;) {
            if (!node->target || !node)
                break;

            printf("[    ] clean %s\n", node->target->output);
            remove(node->target->output);

            node = node->next;
        }
    } else {
        printf("[    ] invalid action: %s\n", argv[1]);
        quit(1);
    }

    quit(0);
    return 0;
}

    #define main(...) wbuild_main(int argc, char** argv)
#endif

#endif
