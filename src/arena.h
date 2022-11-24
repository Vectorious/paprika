#pragma once


#include "paprika.h"


struct Memory_Arena
{
    mem_t size;
    mem_t used;
    u8 *base;
};

#define ArenaAllocStruct(arena, type) ((type *)ArenaAlloc(arena, sizeof(type)))
#define ArenaAllocStructAndZero(arena, type) ((type *)ArenaAlloc(arena, sizeof(type), true))
#define ArenaAllocStructArray(arena, type, count) ((type *)ArenaAlloc(arena, count * sizeof(type)))

internal inline u8 *
ArenaAlloc(Memory_Arena *arena, mem_t size, bool32 zero_memory = false)
{
    u8 *result = arena->base + arena->used;
    arena->used += size;

    Assert(arena->used <= arena->size);

    if (zero_memory)
    {
        MemZero(result, size);
    }

    return result;
}

// NOTE: Includes null terminator.
internal char *
ArenaAllocString(Memory_Arena *arena, char *src)
{
    char *result = (char *)(arena->base + arena->used);

    u32 size = 1;
    char *dst = result;
    while (*src)
    {
        *dst++ = *src++;
        ++size;
    }

    *dst = '\0';

    arena->used += size;

    Assert(arena->used <= arena->size);

    return result;
}

// NOTE: Does not include null terminator.
internal char *
ArenaPushString(Memory_Arena *arena, char *src)
{
    char *result = (char *)(arena->base + arena->used);

    u32 size = 0;
    char *dst = result;
    while (*src)
    {
        *dst++ = *src++;
        ++size;
    }

    arena->used += size;

    Assert(arena->used <= arena->size);

    return result;
}

internal inline void
ArenaReset(Memory_Arena *arena)
{
    arena->used = 0;
}

internal inline void
ArenaInitialize(Memory_Arena *arena, u8 *base, mem_t size)
{
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}

internal inline void
ArenaSub(Memory_Arena *arena, Memory_Arena *sub_arena, mem_t size)
{
    ArenaInitialize(sub_arena,
                    ArenaAlloc(arena, size),
                    size);
}

// NOTE: The location of this arena's memory could change, so don't store raw pointers to
// any of its contents.
struct Growable_Memory_Arena
{
    mem_t size;
    mem_t used;
    u8 *base;
};

#ifndef ArenaAllocImplemented
#include "stdlib.h"
#define MemAlloc malloc
#define MemRealloc realloc
#endif

#ifndef ALLOC_INCREMENT
#define ALLOC_INCREMENT Megabytes(1)
#endif

#define GrowableArenaAllocStruct(arena, type) (GrowableArenaAlloc(arena, sizeof(type)))
#define GrowableArenaAllocStructAndZero(arena, type) (GrowableArenaAlloc(arena, sizeof(type), true))
#define GrowableArenaAllocStructArray(arena, type, count) (GrowableArenaAlloc(arena, count * sizeof(type)))

#define GrowableArenaGetStruct(arena, type, offset) ((type *)GrowableArenaGet(arena, offset))
#define GrowableArenaGetString(arena, offset) ((char *)GrowableArenaGet(arena, offset))

#define GrowableArenaAllocAndGet(arena, size) (GrowableArenaGet(arena, GrowableArenaAlloc(arena, size)))
#define GrowableArenaAllocStructAndGet(arena, type) (GrowableArenaGetStruct(arena, type, GrowableArenaAllocStruct(arena, type)))
#define GrowableArenaAllocStringAndGet(arena, src) (GrowableArenaGetString(arena, GrowableArenaAllocString(arena, src)))

typedef mem_t Arena_Offset;

internal inline u8 *
GrowableArenaGet(Growable_Memory_Arena *arena, Arena_Offset offset)
{
    u8 *result = 0;
    if (offset)
        result = arena->base + offset - 1;
    return result;
}

internal inline void
GrowableArenaResize(Growable_Memory_Arena *arena, mem_t target_size)
{
    if (target_size > arena->size)
    {
        arena->size = (target_size / ALLOC_INCREMENT + 1) * ALLOC_INCREMENT;
        arena->base = (u8 *)MemRealloc(arena->base, arena->size);
    }
}

internal inline Arena_Offset
GrowableArenaAlloc(Growable_Memory_Arena *arena, mem_t size, bool32 zero_memory = false)
{
    mem_t new_used = arena->used + size;
    
    GrowableArenaResize(arena, new_used);

    u8 *result = arena->base + arena->used;
    arena->used = new_used;
    
    if (zero_memory)
    {
        MemZero(result, size);
    }

    return result - arena->base + 1;
}

internal Arena_Offset
GrowableArenaAllocString(Growable_Memory_Arena *arena, char *src)
{
    mem_t size = StringLength(src);
    char *dst = (char *)GrowableArenaAllocAndGet(arena, size);
    MemCopy(src, dst, size);
    Arena_Offset result = (u8 *)dst - arena->base + 1;
    return result;
}

internal inline void
GrowableArenaInitialize(Growable_Memory_Arena *arena, mem_t size = 0)
{
    if (size)
    {
        arena->base = (u8 *)MemAlloc(size);
    }
    else
    {
        arena->base = 0;
    }
    arena->size = size;
    arena->used = 0;
}

internal inline void
GrowableArenaReset(Growable_Memory_Arena *arena)
{
    arena->used = 0;
}
