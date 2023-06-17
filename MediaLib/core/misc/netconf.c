/*****************************************************************************
 * netconf.c : Network configuration
 *****************************************************************************/

#include <vlc_fixups.h>
#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#include <vlc_url.h>

#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
char *vlc_getProxyUrl(const char *psz_url)
{
    VLC_UNUSED(psz_url);

    char *proxy = config_GetPsz((vlc_object_t *)(NULL), "http-proxy");
    if (proxy == NULL)
        return NULL;

    char *proxy_pwd = config_GetPsz((vlc_object_t *)(NULL), "http-proxy-pwd");
    if (proxy_pwd == NULL)
        return proxy;

    vlc_url_t url;
    if (vlc_UrlParse(&url, proxy) < 0) {
        vlc_UrlClean(&url);
        free(proxy);
        free(proxy_pwd);
        return NULL;
    }

    if (url.psz_password == NULL)
        url.psz_password = proxy_pwd;

    char *proxy_url = vlc_uri_compose(&url);
    vlc_UrlClean(&url);

    free(proxy_pwd);
    free(proxy);

#if 0
    /* Try to get the proxy server address from Windows internet settings. */
    HKEY h_key;

    /* Open the key */
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft"
        "\\Windows\\CurrentVersion\\Internet Settings",
        0, KEY_READ, &h_key) == ERROR_SUCCESS)
        return NULL;

    DWORD len = sizeof(DWORD);
    BYTE proxyEnable;

    /* Get the proxy enable value */
    if (RegQueryValueEx(h_key, "ProxyEnable", NULL, NULL,
        &proxyEnable, &len) != ERROR_SUCCESS
        || !proxyEnable)
        goto out;

    /* Proxy is enabled */
    /* Get the proxy URL :
    Proxy server value in the registry can be something like "address:port"
    or "ftp=address1:port1;http=address2:port2 ..."
    depending of the configuration. */
    unsigned char key[256];

    len = sizeof(key);
    if (RegQueryValueEx(h_key, "ProxyServer", NULL, NULL,
        key, &len) == ERROR_SUCCESS)
    {
        /* FIXME: This is lame. The string should be tokenized. */
        #warning FIXME.
        char *psz_proxy = strstr((char *)key, "http=");
        if (psz_proxy != NULL)
        {
            psz_proxy += 5;
            char *end = strchr(psz_proxy, ';');
            if (end != NULL)
                *end = '\0';
        }
        else
            psz_proxy = (char *)key;
        proxy_url = strdup(psz_proxy);
    }

out:
    RegCloseKey(h_key);
#endif
    return proxy_url;
}
#elif defined(__ANDROID__)
#include <jni.h>

char *vlc_getProxyUrl(const char *url)
{
#if 0
    VLC_UNUSED(url);
    JNIEnv *env;
    bool b_detach;
    char *psz_ret = NULL;
    const char *psz_host = NULL, *psz_port = NULL;
    jstring jhost = NULL, jport = NULL;

    env = get_env(&b_detach);
    if (env == NULL)
        return NULL;

    jstring jkey = (*env)->NewStringUTF(env, "http.proxyHost");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        jkey = NULL;
    }
    if (jkey == NULL)
        goto end;

    jhost = (*env)->CallStaticObjectMethod(env, fields.System.clazz, fields.System.getProperty, jkey);
    (*env)->DeleteLocalRef(env, jkey);
    if (jhost == NULL)
        goto end;

    psz_host = (*env)->GetStringUTFChars(env, jhost, 0);
    if (psz_host == NULL || psz_host[0] == '\0')
        goto end;

    jkey = (*env)->NewStringUTF(env, "http.proxyPort");
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionClear(env);
        jkey = NULL;
    }
    if (jkey == NULL)
        goto end;

    jport = (*env)->CallStaticObjectMethod(env, fields.System.clazz, fields.System.getProperty, jkey);
    (*env)->DeleteLocalRef(env, jkey);

    if (jport != NULL)
    {
        psz_port = (*env)->GetStringUTFChars(env, jport, 0);
        if (psz_port != NULL && (psz_port[0] == '\0' || psz_port[0] == '0'))
        {
            (*env)->ReleaseStringUTFChars(env, jport, psz_port);
            psz_port = NULL;
        }
    }

    if (asprintf(&psz_ret, "http://%s%s%s",
        psz_host,
        psz_port != NULL ? ":" : "",
        psz_port != NULL ? psz_port : "") == -1)
        psz_ret = NULL;

end:
    if (psz_host != NULL)
        (*env)->ReleaseStringUTFChars(env, jhost, psz_host);
    if (jhost != NULL)
        (*env)->DeleteLocalRef(env, jhost);
    if (psz_port != NULL)
        (*env)->ReleaseStringUTFChars(env, jport, psz_port);
    if (jport != NULL)
        (*env)->DeleteLocalRef(env, jport);
    release_env(b_detach);
    return psz_ret;
#else
    return NULL;
#endif
}
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
//#include <spawn.h>
#include <unistd.h>

extern char **environ;
char *vlc_getProxyUrl(const char *url)
{
    /* libproxy helper */
    //pid_t pid;
    //posix_spawn_file_actions_t actions;
    //posix_spawnattr_t attr;
    //char *argv[3] = { (char *)"proxy", (char *)url, NULL };
    //int fd[2];

    //if (vlc_pipe(fd))
    //    return NULL;

    //if (posix_spawn_file_actions_init(&actions))
    //    return NULL;
    //if (posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null",
    //                                     O_RDONLY, 0644) ||
    //    posix_spawn_file_actions_adddup2(&actions, fd[1], STDOUT_FILENO))
    //{
    //    posix_spawn_file_actions_destroy(&actions);
    //    return NULL;
    //}

    //posix_spawnattr_init(&attr);
    //{
    //    sigset_t set;

    //    sigemptyset(&set);
    //    posix_spawnattr_setsigmask(&attr, &set);
    //    sigaddset (&set, SIGPIPE);
    //    posix_spawnattr_setsigdefault(&attr, &set);
    //    posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGDEF
    //                                  | POSIX_SPAWN_SETSIGMASK);
    //}

    //if (posix_spawnp(&pid, "proxy", &actions, &attr, argv, environ))
    //    pid = -1;

    //posix_spawnattr_destroy(&attr);
    //posix_spawn_file_actions_destroy(&actions);
    //vlc_close(fd[1]);

    //if (pid != -1)
    //{
    //    char buf[1024];
    //    size_t len = 0;

    //    do
    //    {
    //         ssize_t val = read(fd[0], buf + len, sizeof (buf) - len);
    //         if (val <= 0)
    //             break;
    //         len += val;
    //    }
    //    while (len < sizeof (buf));

    //    vlc_close(fd[0]);
    //    while (waitpid(pid, &(int){ 0 }, 0) == -1);

    //    if (len >= 9 && !strncasecmp(buf, "direct://", 9))
    //        return NULL;

    //    char *end = memchr(buf, '\n', len);
    //    if (end != NULL)
    //    {
    //        *end = '\0';
    //        return strdup(buf);
    //    }
    //    /* Parse error: fallback (may be due to missing executable) */
    //}
    //else
    //    vlc_close(fd[0]);

    ///* Fallback to environment variable */
    //char *var = getenv("http_proxy");
    //if (var != NULL)
    //    var = strdup(var);
    //return var;
    return 0;
}
#endif