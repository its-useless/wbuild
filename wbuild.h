#ifndef WBUILD_H
#define WBUILD_H
#include <stdio.h>
#include <stdlib.h>

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

#ifdef WBUILD_IMPL
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

/// TARGET
typedef struct {
    char* output;

    char** depends;
    size_t depends_len;

    char** commands;
    size_t commands_len;
} Target;

typedef struct TargetNode {
    Target* target;
    struct TargetNode* next;
} TargetNode;

static Arena global_arena;
static TargetNode targets = {0};
    #define aalloc(size) arena_alloc(&global_arena, size)

static void quit(int code) {
    arena_free(&global_arena);
    exit(code);
}

void add_target(char* output) {
    Target* target = aalloc(sizeof(Target));

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
        if (!node->target)
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

    #define target_depends(target, ...) \
        _target_depends(target, (const char* const[]) {__VA_ARGS__, NULL});
    #define target_commands(target, ...) \
        _target_commands(target, (const char* const[]) {__VA_ARGS__, NULL});

int wbuild_main(int, char**);

int main(int argc, char** argv) {
    int res;

    global_arena = arena_new(0x1000);

    puts("[...] useless/wbuild aceinetx 2021-2025");
    if (argc < 2) {
        puts("[...] no action provided, actions: build | clean");
        quit(1);
    }

    if (strcmp(argv[1], "build") == 0) {
        if ((res = wbuild_main(argc, argv)) != 0) {
            quit(res);
        }
    } else if (strcmp(argv[1], "clean") == 0) {
    } else {
        printf("[...] invalid action: %s", argv[1]);
        quit(1);
    }

    quit(0);
    return 0;
}

    #define main(...) wbuild_main(int argc, char** argv)
#endif

#endif
