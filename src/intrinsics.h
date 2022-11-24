#pragma once


#include "paprika.h"


struct Find_Bit_Result
{
    bool32 found;
    u32 index;
};

internal inline Find_Bit_Result
FindLeastSignificantSetBit(u32 value)
{
    Find_Bit_Result result = {};

#if COMPILER_MSVC
    result.found = _BitScanForward((unsigned long *)&result.index, value);
#else
    for (u32 index = 0; index < 32; ++index)
    {
        if (value & (1 << index))
        {
            result.index = index;
            result.found = true;
            break;
        }
    }
#endif

    return result;
}

internal inline void
MemCopy(void *src, void *dst, mem_t size)
{
#if COMPILER_MSVC
    __movsb((u8 *)dst, (u8 *)src, size);
#else
    u8 *src_byte = (u8 *)src;
    u8 *dst_byte = (u8 *)dst;

    for (u32 byte_idx = 0; byte_idx < size; ++byte_idx)
    {
        *dst_byte++ = *src_byte++;
    }
#endif
}

#if 0
// NOTE: Untested.
internal inline void
MemFill(void *src, void *dst, mem_t size, mem_t count)
{
 while (mem_t i = 0; i < count; ++i)
 {
#if COMPILER_MSVC
  __movsb((u8 *)dst, (u8 *)src, size);
#else
  u8 *src_byte = (u8 *)src;
  u8 *dst_byte = (u8 *)dst;
  
  for (u32 byte_idx = 0; byte_idx < size; ++byte_idx) {
   *dst_byte++ = *src_byte++;
  }
#endif
  dst += size;
 }
}
#endif

internal inline void
MemZero(void *mem, mem_t size)
{
    u32 size_32 = (u32)(size / 4);
    u32 extra_bytes = (u32)(size - size_32 * 4);

    u32 *ptr = (u32 *)mem;
    for (u32 copy_idx = 0; copy_idx < size_32; ++copy_idx)
    {
        *ptr++ = 0;
    }

    u8 *extra_ptr = (u8 *)ptr;
    for (u32 copy_idx = 0; copy_idx < extra_bytes; ++copy_idx)
    {
        *extra_ptr++ = 0;
    }
}

internal inline f32
Absolute(f32 a)
{
    return fabsf(a);
}

internal inline f32
SquareRt(f32 a)
{
    return sqrtf(a);
}

internal inline f32
Square(f32 a)
{
    f32 result = a * a;
    return result;
}

internal inline bool32
StringCmp(char *a, char *b)
{
    while (*a && *b)
    {
        if (*a++ != *b++)
        {
            return false;
        }
    }
    return *a == *b;
}

internal inline bool32
StringCmpLen(char *a, char *b, u32 length)
{
    for (u32 idx = 0; idx < length; ++idx)
    {
        if (*a == '\0' || *a++ != *b++)
        {
            return false;
        }
    }
    return true;
}

// NOTE: Includes the null terminator.
internal inline mem_t
StringLength(char *s)
{
    mem_t result = 1;
    while (*s++ != '\0')
    {
        ++result;
    }
    return result;
}

// NOTE: Ignores all non-digit characters except for leading '-'.
internal inline i32
StringToI32(char *start, char *end = 0)
{
    i32 result = 0;
    i32 multiplier = 1;
    if (*start == '-')
    {
        ++start;
        multiplier = -1;
    }

    while (start < end || (!end && *start))
    {
        if ('0' <= *start && *start <= '9')
        {
            result = 10 * result + (*start - '0');
        }

        ++start;
    }

    result *= multiplier;
    return result;
}

// NOTE: Ignores all non-digit characters except for leading '-'.
internal i64
StringToI64(char *start, char *end = 0)
{
    i64 result = 0;
    i32 multiplier = 1;
    if (*start == '-')
    {
        ++start;
        multiplier = -1;
    }

    while (start < end || (!end && *start))
    {
        if ('0' <= *start && *start <= '9')
        {
            result = 10 * result + (*start - '0');
        }

        ++start;
    }

    result *= multiplier;
    return result;
}

// NOTE: Ignores all non-digit characters except for leading '-'.
internal f64
StringToF64(char *start, char *end = 0)
{
    f64 result = 0.0;

    bool32 negative = *start == '-';
    u64 past_decimal = 0;

    while (start < end || (!end && *start))
    {
        if ('0' <= *start && *start <= '9')
        {
            u32 digit = *start - '0';
            if (past_decimal)
            {
                result += digit / past_decimal;
                past_decimal *= 10;
            }
            else
            {
                result = 10 * result + digit;
            }
        }
        else if (*start == '.')
        {
            past_decimal = 10;
        }

        ++start;
    }

    if (negative)
        result = -result;

    return result;
}

internal inline void
StringCopy(char *src, char *dst)
{
    while (*src)
    {
        *dst++ = *src++;
    }
    *dst = '\0';
}
