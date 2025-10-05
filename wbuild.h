#ifndef WBUILD_H
#define WBUILD_H
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    void* data;
    size_t size;
    void* next;
} Arena;

Arena arena_new(size_t size);
void* arena_alloc(Arena* arena, size_t size);
void arena_free(Arena* arena);

#ifdef WBUILD_IMPL
Arena arena_new(size_t size) {
    Arena arena;
    arena.next = arena.data = malloc(size);
    arena.size = size;
    return arena;
}

void* arena_alloc(Arena* arena, size_t size) {
    void* p = arena->next;
    arena->next += size;
    return p;
}

void arena_free(Arena* arena) {
    free(arena->data);
}

int wbuild_main(int, char**);

int main(int argc, char** argv) {
    int res;

    puts("[...] useless/wbuild aceinetx 2021-2025");
    if ((res = wbuild_main(argc, argv)) != 0) {
        return res;
    }

    return 0;
}

    #define main wbuild_main
#endif

#endif
