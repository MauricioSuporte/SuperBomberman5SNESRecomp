#ifndef SB5_ANDROID_JAVA_H
#define SB5_ANDROID_JAVA_H
/* Android only (android_java.c): HTTPS and the in-place update, done by the
 * app's Activity because Android has no curl or updater executable. */
#include <stdbool.h>
#include <stddef.h>

/* One HTTPS request (body may be NULL). Returns the HTTP status (0 when the
 * request failed) and copies the response body into out. Blocking: call it
 * from a worker thread. */
int Sb5AndroidHttp(const char *method, const char *url, const char *body, char *out, size_t cap);
/* Starts downloading and installing the APK at url over this app; msg gets
 * the line to show. Returns false when the Activity could not be reached. */
bool Sb5AndroidInstallApk(const char *url, char *msg, size_t cap);
/* Latest progress line of that update (empty before one starts). */
bool Sb5AndroidUpdateStatus(char *msg, size_t cap);

#endif
