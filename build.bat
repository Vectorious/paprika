@echo off

if "_%VisualStudioVersion%_" == "_2022_" goto vsinpath
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:vsinpath

set RELEASE="-O2 -DRELEASE_BUILD -Wno-undef -MT"
set INTERNAL="-Od -DINTERNAL_BUILD -Wno-null-dereference -MDd"

set COMMON_CL="-fdiagnostics-absolute-paths -fcolor-diagnostics -fansi-escape-codes -nologo -EHa -Gm- -Oi -GR- -wd4201 -wd4100 -wd4005 -wd4505 -wd4189 -FC -Z7 -Qunused-arguments -I../include"
set CLANG_COMMON="-fdiagnostics-absolute-paths -I../include -L../lib"
set WIN32_INCLUDE="-luser32 -lGdi32 -lwinmm -lshell32 -lopengl32 -lglfw3_mt"
set CURL_INCLUDE="-llibssl -llibcrypto -lzlibd -ladvapi32 -lcrypt32 -lws2_32 -llibcurl-d"
set CL_WIN32_INCLUDE="user32.lib Gdi32.lib winmm.lib shell32.lib opengl32.lib glfw3_mt.lib"
set CL_CURL_INCLUDE="libcrypto.lib zlibd.lib advapi32.lib crypt32.lib ws2_32.lib libssl.lib libcurl-d.lib"
set CL_CURL_STATIC_INCLUDE="libcrypto.lib zlib.lib advapi32.lib crypt32.lib ws2_32.lib libssl.lib libcurl.lib"
set RELEASE_CL_CURL_INCLUDE="advapi32.lib crypt32.lib ws2_32.lib release/libcrypto.lib release/zlib.lib release/libssl.lib release/libcurl.lib"
set RELEASE_CL_STATIC_INCLUDE="advapi32.lib crypt32.lib ws2_32.lib release_static/libcrypto.lib release_static/zlib.lib release_static/libssl.lib release_static/libcurl.lib"
set WARNINGS="-Weverything -Wno-writable-strings -Wno-unused-function -Wno-missing-braces -Wno-pointer-to-int-cast -Wno-sometimes-uninitialized -Wno-cast-function-type -Wno-old-style-cast -Wno-c++98-compat -Wno-zero-as-null-pointer-constant -Wno-missing-prototypes -Wno-double-promotion -Wno-extra-semi-stmt -Wno-switch-enum -Wno-cast-align -Wno-c++98-compat-pedantic -Wno-implicit-int-float-conversion -Wno-float-conversion -Wno-undef -Wno-sign-conversion -Wno-gnu-anonymous-struct"

set RANDOM_VAL="%RANDOM%"


set RELEASE_BUILD=0
set USE_EMU=1

IF %RELEASE_BUILD%==0 (
    if %USE_EMU%==0 (
        echo Building in debug mode...

        mkdir bin 2>nul
        pushd bin

        del *.pdb

        clang-cl "%WARNINGS%" "%COMMON_CL%" "%INTERNAL%" -DCURL_STATICLIB -Fmpaprika.map ../src/paprika.cpp -LD -link -noimplib -noexp -libpath:../lib -incremental:no -PDB:paprika-%RANDOM_VAL%.pdb -nodefaultlib:LIBCMTD -OUT:paprika_build.dll && move paprika_build.dll paprika.dll >nul
        clang-cl "%WARNINGS%" "%COMMON_CL%" "%INTERNAL%" -Fmpaprika_win32.map -DCURL_STATICLIB ../src/paprika_win32.cpp -link "%CL_WIN32_INCLUDE%" "%CL_CURL_INCLUDE%" -libpath:../lib -incremental:no -nodefaultlib:LIBCMTD -nodefaultlib:LIBCMT -OUT:paprika_win32.exe
    ) else (
        echo Building in test mode...

        mkdir test\bin 2>nul
        pushd test

        del bin\*.pdb

        clang-cl "%WARNINGS%" "%COMMON_CL%" "%INTERNAL%" -DUSE_EMU -DCURL_STATICLIB -Fmpaprika.map ../src/paprika.cpp -LD -link -noimplib -noexp -libpath:../lib -incremental:no -PDB:bin/paprika-%RANDOM_VAL%.pdb -nodefaultlib:LIBCMTD -OUT:paprika_build.dll && move paprika_build.dll bin/paprika.dll >nul
        clang-cl "%WARNINGS%" "%COMMON_CL%" "%INTERNAL%" -DUSE_EMU -Fmpaprika_win32.map -DCURL_STATICLIB ../src/paprika_win32.cpp -link "%CL_WIN32_INCLUDE%" "%CL_CURL_INCLUDE%" -libpath:../lib -incremental:no -nodefaultlib:LIBCMTD -nodefaultlib:LIBCMT -OUT:bin/paprika_win32.exe
    )
) else (
    echo Building in release mode...

    mkdir bin\release 2>nul
    pushd bin

    del release\*.pdb

    clang-cl "%WARNINGS%" "%COMMON_CL%" "%RELEASE%" -Fmpaprika_win32.map -DCURL_STATICLIB ../src/paprika_win32.cpp -link -subsystem:WINDOWS "%CL_WIN32_INCLUDE%" "%RELEASE_CL_STATIC_INCLUDE%" -libpath:../lib -incremental:no -OUT:release/paprika_win32.exe
)

popd