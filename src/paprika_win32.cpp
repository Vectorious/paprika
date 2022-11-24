#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <stdio.h>
#include <Windows.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "paprika_types.h"

#define IM_VEC2_CLASS_EXTRA                                                     \
        constexpr ImVec2(const Vector2& v) : x(v.x), y(v.y) {}                  \
        operator Vector2() const { return {x,y}; }
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

#ifdef INTERNAL_BUILD
#include "imgui/imgui_demo.cpp"
#endif

#include "imgui/imgui_impl_glfw.cpp"
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#include "imgui/imgui_impl_opengl3.cpp"

#include "implot/implot.h"
#include "implot/implot_internal.h"
#include "implot/implot.cpp"
#include "implot/implot_items.cpp"
#pragma clang diagnostic push

#include "paprika.h"

#include "saltybet.cpp"

#ifdef RELEASE_BUILD
#include "paprika.cpp"
#endif


struct Paprika_Functions
{
    Paprika_Update_Function *Update;
    Paprika_Stop_Running_Function *StopRunning;
    
    HMODULE handle;
    FILETIME last_write_time;
};


internal PlatformReadFile(Win32ReadFile)
{
    Read_File_Result result = {};

    FILE *file = 0;
    fopen_s(&file, filename, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        result.size = (mem_t)ftell(file);
        fseek(file, 0, SEEK_SET);

        result.data = (u8 *)malloc(result.size);
        fread(result.data, result.size, 1, file);
        fclose(file);
    }

    return result;
}

internal void
Win32WriteEntireFile(char *filename, u8 *data, mem_t size)
{
    FILE *file = 0;
    if (fopen_s(&file, filename, "wb"))
    {
        fwrite(data, size, 1, file);
        fclose(file);
    }
}

internal PlatformFileWrite(Win32FileWrite)
{
    return fwrite(buffer, element_size, element_count, (FILE *)handle);
}

internal PlatformFileClose(Win32FileClose)
{
    return fclose((FILE *)handle);
}

internal PlatformWriteFile(Win32WriteFile)
{
    Write_File_Result result = {};
    fopen_s((FILE **)&result.handle, filename, "wb");
    return result;
}

internal PlatformWriteFile(Win32AppendFile)
{
    Write_File_Result result = {};
    fopen_s((FILE **)&result.handle, filename, "ab");
    return result;
}

internal u32
Win32GetTimestamp()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    u64 t = ft.dwLowDateTime | ((u64)ft.dwHighDateTime << 32);
    // NOTE: t = 100-nanosecond intervals since January 1, 1601 (UTC).
    // We divide by 10,000,000 to convert to seconds, and subtract the difference
    // between January 1, 1970 and January 1, 1601 (11,644,473,600 seconds).
    const u64 hundred_nanoseconds_per_second = 10000000;
    const u64 filetime_to_unix_difference = 11644473600;
    t = t / hundred_nanoseconds_per_second - filetime_to_unix_difference;
    return (u32)t;
}

internal inline LARGE_INTEGER
Win32InitializeCounter()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

internal inline f32
Win32GetCounterElapsed(LARGE_INTEGER counter, f32 counter_frequency)
{
    return (f32)(Win32InitializeCounter().QuadPart - counter.QuadPart) / counter_frequency;
}

internal inline f32
Win32UpdateCounterAndGetSeconds(LARGE_INTEGER *counter, f32 counter_frequency)
{
    i64 old_counter = counter->QuadPart;
    QueryPerformanceCounter(counter);
    i64 difference = counter->QuadPart - old_counter;
    return (f32)difference / counter_frequency;
}

internal void
Win32PrepareFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

internal Paprika_Platform
Win32GetPlatform()
{
    Paprika_Platform result;

    result.ReadFile = Win32ReadFile;
    result.WriteFile = Win32WriteFile;
    result.AppendFile = Win32AppendFile;
    
    result.MemFree = free;
    
    result.FileWrite = Win32FileWrite;
    result.FileClose = Win32FileClose;

    result.GetTimestamp = Win32GetTimestamp;

    result.PrepareFrame = Win32PrepareFrame;

    result.im_ctx = ImGui::GetCurrentContext();
    result.implot_ctx = ImPlot::GetCurrentContext();

    return result;
}

internal inline FILETIME
Win32GetLastWriteTime(char *filename)
{
    FILETIME result = {};
 
    WIN32_FILE_ATTRIBUTE_DATA file_attributes;
    if (GetFileAttributesExA((LPCSTR)filename, GetFileExInfoStandard, &file_attributes)) {
        result = file_attributes.ftLastWriteTime;
    }
 
    return result;
}

internal Paprika_Functions
Win32LoadPaprikaCode()
{
    Paprika_Functions result = {};

    CopyFileA("paprika.dll", "paprika_active.dll", FALSE);
    result.handle = LoadLibraryA("paprika_active.dll");
    if (result.handle)
    {
        result.Update = (Paprika_Update_Function *)GetProcAddress(result.handle, "PaprikaUpdate");
        result.StopRunning = (Paprika_Stop_Running_Function *)GetProcAddress(result.handle, "PaprikaStopRunning");

        result.last_write_time = Win32GetLastWriteTime("paprika.dll");
    }
    return result;
}

internal void
Win32UnloadPaprikaCode(Paprika_Functions *paprika_lib)
{
    paprika_lib->Update = PaprikaUpdateStub;
    paprika_lib->StopRunning = PaprikaStopRunningStub;
    paprika_lib->last_write_time = {};
    if (paprika_lib->handle)
    {
        FreeLibrary(paprika_lib->handle);
        paprika_lib->handle = 0;
    }
}

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    if (glfwInit() == GLFW_TRUE)
    {
        const char *glsl_version = "#version 330 core";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow *window = glfwCreateWindow(1280, 720, "paprikabot2", NULL, NULL);
        if (window)
        {
            glfwMakeContextCurrent(window);
            
            // Enables vsync
            glfwSwapInterval(1);

            gladLoadGL((GLADloadfunc) glfwGetProcAddress);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImPlot::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;

            ImGui::StyleColorsDark();

            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init(glsl_version);

            ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

#ifdef INTERNAL_BUILD
            Paprika_Functions paprika_lib = Win32LoadPaprikaCode();
#endif

            Paprika_State paprika = {};
            paprika.platform = Win32GetPlatform();
            mem_t paprika_arena_size = Megabytes(8);
            ArenaInitialize(&paprika.arena, (u8 *)malloc(paprika_arena_size), paprika_arena_size);
            SaltyBetClientInit(&paprika.client);

            LARGE_INTEGER counter = Win32InitializeCounter();

            f32 counter_frequency;
            {
                LARGE_INTEGER counter_frequency_result;
                QueryPerformanceFrequency(&counter_frequency_result);
                counter_frequency = (f32)counter_frequency_result.QuadPart;
            }

            Paprika_Input input = {};

            bool32 running = true;
            while (running && !glfwWindowShouldClose(window))
            {
#ifdef INTERNAL_BUILD
                {
                    FILETIME disk_last_write_time = Win32GetLastWriteTime("paprika.dll");
                    if (CompareFileTime(&disk_last_write_time, &paprika_lib.last_write_time) != 0)
                    {
                        printf("Reloading paprika code...\n");
                        LogWrite(&paprika.platform, "Reloading paprika code...");
                        Win32UnloadPaprikaCode(&paprika_lib);
                        paprika_lib = Win32LoadPaprikaCode();
                    }
                }
#endif

                glfwPollEvents();

                if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
                    paprika.frame_rate = 0.0f;
                else
                    paprika.frame_rate = 1.0f / 15.0f;

                input = {};
                input.time_delta = Win32UpdateCounterAndGetSeconds(&counter, counter_frequency);

#ifdef INTERNAL_BUILD
                bool32 rendered = paprika_lib.Update(&paprika, input);
#else
                bool32 rendered = PaprikaUpdate(&paprika, input);
#endif

                if (rendered)
                {
                    int display_w, display_h;
                    glfwGetFramebufferSize(window, &display_w, &display_h);
                    glViewport(0, 0, display_w, display_h);
                    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
                    glClear(GL_COLOR_BUFFER_BIT);
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                    glfwSwapBuffers(window);
                }

                if (paprika.frame_rate)
                {
                    u64 sleep_ms = (paprika.frame_rate - paprika.frame_counter) * 1000.0f;
                    Sleep(sleep_ms);
                }
            }

#ifdef INTERNAL_BUILD
            paprika_lib.StopRunning(&paprika);
#else
            PaprikaStopRunning(&paprika);
#endif

#if 0
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            // NOTE: I don't think this is a necessary call, and it causes issues due to
            // the platform layer and the main layer having separate heaps when the main
            // layer is being loaded dynamically.
            ImPlot::DestroyContext();
            ImGui::DestroyContext();
#endif

            glfwDestroyWindow(window);
            glfwTerminate();
            return 0;
        }
        else
        {
            glfwTerminate();
            return -1;
        }
    }
    else
    {
        return -1;
    }
}
