/*****************************************************************************
 * specific.c: Win32/Android/Linux specific initialization
 *****************************************************************************/

#ifndef UNICODE
#define UNICODE
#endif

#include "libvlc_internal.h"
#include <vlc_internal.h>

#ifdef _WIN32
#include <vlc_getopt.h>
#include <mmsystem.h>

static int system_InitWSA(int hi, int lo)
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(hi, lo), &data) == 0)
    {
        if (LOBYTE(data.wVersion) == 2 && HIBYTE(data.wVersion) == 2)
            return 0;
        WSACleanup( );
    }
    return -1;
}

void system_Init(void)
{
    if (system_InitWSA(2, 2) && system_InitWSA(1, 1))
        fputs("Error: cannot initialize widows sockets\n", stderr);
}

void system_Configure(libvlc_int_t *p_this)
{
}

void system_End(void)
{
    WSACleanup();
}
#elif defined(__ANDROID__)
#include <jni.h>
#include <assert.h>

static JavaVM* s_jvm = NULL;

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    s_jvm = vm;
    //msg_Info(NULL, "===================== JNI_OnLoad ==================");
    return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved)
{
    (void) reserved;
}

void system_Init(void)
{
}

void system_Configure(libvlc_int_t *p_libvlc)
{
    assert(s_jvm != NULL);
    var_Create(p_libvlc, "android-jvm", VLC_VAR_ADDRESS);
    var_SetAddress(p_libvlc, "android-jvm", s_jvm);
}
#else
void system_Init(void)
{
}

void system_Configure(libvlc_int_t *libvlc)
{
}
#endif
