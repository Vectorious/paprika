#pragma once


#include "paprika.h"


struct Write_File_Result
{
    void *handle;
};

struct Read_File_Result
{
    u8 *data;
    mem_t size;
};


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#define PlatformMemFree(name) void name(void *ptr)
typedef PlatformMemFree(Platform_Mem_Free);
internal PlatformMemFree(MemFreeStub) {}

#define PlatformFileWrite(name) mem_t name(void *buffer, mem_t element_size, mem_t element_count, void *handle)
typedef PlatformFileWrite(Platform_File_Write);
internal PlatformFileWrite(FileWriteStub) { return 0; }

#define PlatformFileClose(name) i32 name(void *handle)
typedef PlatformFileClose(Platform_File_Close);
internal PlatformFileClose(FileCloseStub) { return 0; }

#define PlatformGetTimestamp(name) u32 name()
typedef PlatformGetTimestamp(Platform_Get_Timestamp);
internal PlatformGetTimestamp(GetTimestampStub) { return 0; }

#define PlatformReadFile(name) Read_File_Result name(char *filename)
typedef PlatformReadFile(Platform_Read_File);
internal PlatformReadFile(ReadFileStub) { return {}; }

#define PlatformWriteFile(name) Write_File_Result name(char *filename)
typedef PlatformWriteFile(Platform_Write_File);
internal PlatformWriteFile(WriteFileStub) { return {}; }

internal PlatformWriteFile(AppendFileStub) { return {}; }

#define PlatformPrepareFrame(name) void name()
typedef PlatformPrepareFrame(Platform_Prepare_Frame);
internal PlatformPrepareFrame(PlatformPrepareFrameStub) {}


#pragma clang diagnostic pop


struct Paprika_Platform
{
    Platform_Read_File *ReadFile;
    Platform_Write_File *WriteFile;
    Platform_Write_File *AppendFile;

    Platform_Mem_Free *MemFree;

    Platform_File_Write *FileWrite;
    Platform_File_Close *FileClose;

    Platform_Get_Timestamp *GetTimestamp;

    Platform_Prepare_Frame *PrepareFrame;

    ImGuiContext *im_ctx;
    ImPlotContext *implot_ctx;

    struct Paprika_Log *log;
};
