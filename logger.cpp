#include <cstdio>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>

#define SIZE 4

using namespace std;

static int     (*old_chmod)(const char *, mode_t);
static int     (*old_chown)(const char *, uid_t, gid_t);
static int     (*old_close)(int);
static int     (*old_creat)(const char *, mode_t);
static int     (*old_fclose)(FILE *);
static FILE*   (*old_fopen)(const char *, const char *);
static size_t  (*old_fread)(void *, size_t, size_t, FILE *);
static size_t  (*old_fwrite)(const void *, size_t, size_t, FILE *);
static int     (*old_open)(const char *, int, ...);
static ssize_t (*old_read)(int, void *, size_t);
static int     (*old_remove)(const char *);
static int     (*old_rename)(const char *, const char *);
static FILE*   (*old_tmpfile)(void);
static ssize_t (*old_write)(int, const void *, size_t);

extern "C" {
// According to fd or FILE to get the filename by readlink function.
    void get_filename(char *filename, char *path, int fd, FILE *stream) {
        memset(path, 0, PATH_MAX); memset(filename, 0, PATH_MAX);

        if (fd != 0) {
            snprintf(path, PATH_MAX, "/proc/self/fd/%d", fd);
        }

        if (stream != NULL) {
            snprintf(path, PATH_MAX, "/proc/self/fd/%d", fileno(stream));
        }

        if (readlink(path, filename, PATH_MAX) < 0){
            perror("readlink error");
        }
    }

// Change not printable character to a dot.
    void get_string(const void *buf, char *str, int str_size, size_t count) {
        memset(str, 0, str_size); strncpy(str, (const char *) buf, count);

        for (size_t i = 0; i < count; i++) {
            if (!isprint(str[i])) str[i] = '.';
        }
    }

// Below are the monitored library calls.
    int chmod(const char *pathname, mode_t mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_chmod = (int(*)(const char *, mode_t)) dlsym(handle, "chmod");
        int value = old_chmod(pathname, mode); dlclose(handle);

        fprintf(stderr, "[logger] chmod(\"%s\", %o) = %d\n", pathname, mode, value);

        return value;
    }

    int chown(const char *pathname, uid_t owner, gid_t group) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_chown = (int(*)(const char *, uid_t, gid_t)) dlsym(handle, "chown");
        int value = old_chown(pathname, owner, group); dlclose(handle);

        fprintf(stderr, "[logger] chown(\"%s\", %d, %d) = %d\n", pathname, owner, group, value);

        return value;
    }

    int close(int fd) {
        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_close = (int(*)(int)) dlsym(handle, "close");
        int value = old_close(fd); dlclose(handle);

        fprintf(stderr, "[logger] close(\"%s\") = %d\n", filename, value);

        return value;
    }

    int creat(const char *pathname, mode_t mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_creat = (int(*)(const char *, mode_t)) dlsym(handle, "creat");
        int value = old_creat(pathname, mode); dlclose(handle);

        fprintf(stderr, "[logger] creat(\"%s\", %o) = %d\n", pathname, mode, value);

        return value;
    }

    int fclose(FILE *stream) {
        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fclose = (int(*)(FILE *)) dlsym(handle, "fclose");
        int value = old_fclose(stream); dlclose(handle);

        fprintf(stderr, "[logger] fclose(\"%s\") = %d\n", filename, value);

        return value;
    }

    FILE *fopen(const char *pathname, const char *mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fopen = (FILE*(*)(const char *, const char *)) dlsym(handle, "fopen");
        FILE *fp = old_fopen(pathname, mode); dlclose(handle);

        fprintf(stderr, "[logger] fopen(\"%s\", \"%s\") = %p\n", pathname, mode, fp);

        return fp;
    }

    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fread = (size_t(*)(void *, size_t, size_t, FILE *)) dlsym(handle, "fread");
        size_t value = old_fread(ptr, size, nmemb, stream); dlclose(handle);

        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);
        fprintf(stderr, "[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", (char *) ptr, size, nmemb, filename, value);

        return value;
    }

    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fwrite = (size_t(*)(const void *, size_t, size_t, FILE *)) dlsym(handle, "fwrite");
        size_t value = old_fwrite(ptr, size, nmemb, stream); dlclose(handle);

        char str[128]; get_string(ptr, str, sizeof(str), value);
        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);

        fprintf(stderr, "[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", (const char *) ptr, size, nmemb, filename, value);

        return value;
    }

    int open(const char *pathname, int flags, ...) {
// Use va_arg function to check whether there is additional argument or not.
        va_list vl; va_start(vl, flags); mode_t mode = va_arg(vl, mode_t);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_open = (int(*)(const char *, int, ...)) dlsym(handle, "open");
        int value = old_open(pathname, flags, mode); dlclose(handle);

        fprintf(stderr, "[logger] open(\"%s\", %03d, %03o) = %d\n", pathname, flags, mode, value);

        return value;
    }

    ssize_t read(int fd, void *buf, size_t count) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_read = (ssize_t(*)(int, void *, size_t)) dlsym(handle, "read");
        ssize_t value = old_read(fd, buf, count); dlclose(handle);

        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);
        fprintf(stderr, "[logger] read(\"%s\", \"%s\", %ld) = %ld\n", filename, (char *) buf, count, value);

        return value;
    }

    int remove(const char *pathname) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_remove = (int(*)(const char *)) dlsym(handle, "remove");
        int value = old_remove(pathname); dlclose(handle);

        fprintf(stderr, "[logger] remove(\"%s\") = %d\n", pathname, value);

        return value;
    }

    int rename(const char *oldpath, const char *newpath){
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_rename = (int(*)(const char *, const char *)) dlsym(handle, "rename");
        int value = old_rename(oldpath, newpath); dlclose(handle);

        fprintf(stderr, "[logger] rename(\"%s\", \"%s\") = %d\n", oldpath, newpath, value);

        return value;
    }

    FILE *tmpfile(void) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_tmpfile = (FILE *(*)(void)) dlsym(handle, "tmpfile");
        FILE *fp = old_tmpfile(); dlclose(handle);

        fprintf(stderr, "[logger] tmpfile() = %p\n", fp);

        return fp;
    }

    ssize_t write(int fd, const void *buf, size_t count) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_write = (ssize_t(*)(int, const void *, size_t)) dlsym(handle, "write");
        ssize_t value = old_write(fd, buf, count); dlclose(handle);

        char str[128]; get_string(buf, str, sizeof(str), count);
        char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);

        fprintf(stderr, "[logger] write(\"%s\", \"%s\", %ld) = %ld\n", filename, str, value, value);

        return value;
    }
}
