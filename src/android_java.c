/* Android: calls into SB5Activity (android/app/.../SB5Activity.java) for what
 * the desktop build gets from command-line tools. See android_java.h. */
#ifdef __ANDROID__

#include "android_java.h"

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_system.h>

/* Calls `String <name>(<String args...>)` on the activity from any thread
 * (SDL attaches it to the VM). Copies the UTF-8 result into out; returns
 * false when the method is missing, threw, or returned null. */
static bool call_string(const char *name, const char *sig, char *out, size_t cap,
                        int nargs, const char *a0, const char *a1, const char *a2) {
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = env ? (jobject)SDL_AndroidGetActivity() : NULL;
    bool ok = false;
    if (out && cap) out[0] = 0;
    if (!env || !activity) return false;
    if ((*env)->PushLocalFrame(env, 8) != 0) {
        (*env)->DeleteLocalRef(env, activity);
        return false;
    }
    jclass cls = (*env)->GetObjectClass(env, activity);
    jmethodID m = (*env)->GetMethodID(env, cls, name, sig);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); m = NULL; }
    if (m) {
        jstring j0 = a0 ? (*env)->NewStringUTF(env, a0) : NULL;
        jstring j1 = a1 ? (*env)->NewStringUTF(env, a1) : NULL;
        jstring j2 = a2 ? (*env)->NewStringUTF(env, a2) : NULL;
        jstring r = nargs == 0 ? (jstring)(*env)->CallObjectMethod(env, activity, m)
                  : nargs == 1 ? (jstring)(*env)->CallObjectMethod(env, activity, m, j0)
                  : (jstring)(*env)->CallObjectMethod(env, activity, m, j0, j1, j2);
        if ((*env)->ExceptionCheck(env)) {
            (*env)->ExceptionClear(env);
        } else if (r) {
            const char *s = (*env)->GetStringUTFChars(env, r, NULL);
            if (s) {
                if (out && cap) snprintf(out, cap, "%s", s);
                (*env)->ReleaseStringUTFChars(env, r, s);
                ok = true;
            }
        }
    }
    (*env)->PopLocalFrame(env, NULL);
    (*env)->DeleteLocalRef(env, activity);
    return ok;
}

int Sb5AndroidHttp(const char *method, const char *url, const char *body,
                   char *out, size_t cap) {
    if (!out || cap < 2) return 0;
    if (!call_string("sb5Http", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                     out, cap, 3, method, url, body))
        return 0;
    /* "<status>\n<body>" */
    char *nl = strchr(out, '\n');
    int status = atoi(out);
    if (nl) memmove(out, nl + 1, strlen(nl + 1) + 1);
    else out[0] = 0;
    return status;
}

bool Sb5AndroidInstallApk(const char *url, char *msg, size_t cap) {
    return call_string("sb5InstallApk", "(Ljava/lang/String;)Ljava/lang/String;", msg, cap, 1,
                       url, NULL, NULL);
}

bool Sb5AndroidUpdateStatus(char *msg, size_t cap) {
    return call_string("sb5UpdateStatus", "()Ljava/lang/String;", msg, cap, 0, NULL, NULL, NULL);
}

#endif /* __ANDROID__ */
