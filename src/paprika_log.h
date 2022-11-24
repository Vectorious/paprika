#pragma once


#include "paprika.h"


struct Paprika_Log
{
    Line_Buffer line_buffer;
    Memory_Arena write_buffer;

    Paprika_Platform *platform;
};


internal void
LogFlush(Paprika_Log *log)
{
    if (log->write_buffer.used)
    {
        Write_File_Result file = log->platform->AppendFile("log.txt");
        if (file.handle)
        {
            log->platform->FileWrite(log->write_buffer.base, log->write_buffer.used, 1, file.handle);
            log->platform->FileClose(file.handle);
        }

        ArenaReset(&log->write_buffer);
    }
}

internal void
LogWrite(Paprika_Log *log, char *src)
{
    bool32 wrote_src = false;

    u32 str_len = LineBufferAdd(&log->line_buffer, src);
    if (log->write_buffer.used + str_len + 1 > log->write_buffer.size)
    {
        Write_File_Result file = log->platform->AppendFile("log.txt");
        if (file.handle)
        {
            log->platform->FileWrite(log->write_buffer.base, log->write_buffer.used, 1, file.handle);
            
            if (str_len + 1 > log->write_buffer.size)
            {
                log->platform->FileWrite(src, str_len, 1, file.handle);
                log->platform->FileWrite((char *)'\n', 1, 1, file.handle);
                wrote_src = true;
            }

            log->platform->FileClose(file.handle);
        }

        ArenaReset(&log->write_buffer);
    }

    if (!wrote_src)
    {
        MemCopy(src, ArenaAlloc(&log->write_buffer, str_len), str_len);
        *ArenaAlloc(&log->write_buffer, 1) = '\n';
    }
}

internal inline void
LogWrite(Paprika_Platform *platform, char *src)
{
    LogWrite(platform->log, src);
}
