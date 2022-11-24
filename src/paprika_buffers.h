#pragma once


#include "paprika_types.h"


struct Line_Buffer
{
    char *base;
    mem_t size;
    
    mem_t lines;

    mem_t read_offset;
    mem_t write_offset;
};

internal void
LineBufferReset(Line_Buffer *buffer)
{
    buffer->lines = 0;
    buffer->read_offset = 0;
    buffer->write_offset = 0;
}

internal void
LineBufferInit(Line_Buffer *buffer, char *base, mem_t size)
{
    buffer->base = base;
    buffer->size = size;

    LineBufferReset(buffer);
}

internal u32
LineBufferAdd(Line_Buffer *buffer, char *src)
{
    u32 str_len = 0;
    bool32 passed_read = false;
    i32 lines_delta = 1;
    bool32 writing = true;
    while (writing)
    {
        char *write = buffer->base + buffer->write_offset;

        if (passed_read && *write == '\n')
            --lines_delta;

        if (*src)
        {
            ++str_len;
            if (*src == '\n')
                ++lines_delta;

            *write = *src++;
        }
        else
        {
            *write = '\n';
            writing = false;
        }

        buffer->write_offset = (buffer->write_offset + 1) % buffer->size;
        if (buffer->write_offset == buffer->read_offset)
        {
            passed_read = true;
            --lines_delta;
        }
    }

    buffer->lines += lines_delta;

    if (passed_read)
    {
        buffer->read_offset = buffer->write_offset;
        bool32 advancing_read = true;
        while (advancing_read)
        {
            char *read = buffer->base + buffer->read_offset;
            advancing_read = *read != '\n';
            buffer->read_offset = (buffer->read_offset + 1) % buffer->size;
        }
    }

    return str_len;
}

struct Line_Buffer_Read_Result
{
    char *a_start;
    char *a_end;

    char *b_start;
    char *b_end;
};

internal Line_Buffer_Read_Result
LineBufferRead(Line_Buffer *buffer)
{
    Line_Buffer_Read_Result result = {};
    if (buffer->lines)
    {
        result.a_start = buffer->base + buffer->read_offset;
        char *write = buffer->base + buffer->write_offset;
        if (buffer->read_offset < buffer->write_offset)
        {
            result.a_end = write;
        }
        else
        {
            result.a_end = buffer->base + buffer->size;

            result.b_start = buffer->base;
            result.b_end = write;
        }
    }
    return result;
}
