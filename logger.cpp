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

        void change(mode_t mode, char *numbers) {
            int number = mode, binaries[10], count = 8;

            while (number && count >= 0) {
                binaries[count] = number % 2;
                number /= 2; count--;
            }

            while (count >= 0) {
                binaries[count--] = 0;
            }

            for (int i = 0, j = 0; i < 9; i += 3, j++) {
                numbers[j] = '0' + (binaries[i] * 4 + binaries[i + 1] * 2 + binaries[i + 2]);
            }
        }

        int chmod(const char *pathname, mode_t mode) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_chmod = (int(*)(const char *, mode_t)) dlsym(handle, "chmod");
            int value = old_chmod(pathname, mode); dlclose(handle);

            char numbers[SIZE]; memset(numbers, 0, sizeof(numbers)); change(mode, numbers);
            printf("[logger] chmod(\"%s\", %s) = %d\n", pathname, numbers, value);

            return value;
        }

        int chown(const char *pathname, uid_t owner, gid_t group) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_chown = (int(*)(const char *, uid_t, gid_t)) dlsym(handle, "chown");
            int value = old_chown(pathname, owner, group); dlclose(handle);

            printf("[logger] chown(\"%s\", %d, %d) = %d\n", pathname, owner, group, value);

            return value;
        }

        int close(int fd) {
            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);

            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_close = (int(*)(int)) dlsym(handle, "close");
            int value = old_close(fd); dlclose(handle);

            printf("[logger] close(\"%s\") = %d\n", filename, value);

            return value;
        }

        int creat(const char *pathname, mode_t mode) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_creat = (int(*)(const char *, mode_t)) dlsym(handle, "creat");
            int value = old_creat(pathname, mode); dlclose(handle);

            char numbers[SIZE]; memset(numbers, 0, sizeof(numbers)); change(mode, numbers);
            printf("[logger] creat(\"%s\", %s) = %d\n", pathname, numbers, value);

            return value;
        }

        int fclose(FILE *stream) {
            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);

            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_fclose = (int(*)(FILE *)) dlsym(handle, "fclose");
            int value = old_fclose(stream); dlclose(handle);

            printf("[logger] fclose(\"%s\") = %d\n", filename, value);

            return value;
        }

        FILE *fopen(const char *pathname, const char *mode) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_fopen = (FILE*(*)(const char *, const char *)) dlsym(handle, "fopen");
            FILE *fp = old_fopen(pathname, mode); dlclose(handle);

            printf("[logger] fopen(\"%s\", \"%s\") = %p\n", pathname, mode, fp);

            return fp;
        }

        size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_fread = (size_t(*)(void *, size_t, size_t, FILE *)) dlsym(handle, "fread");
            size_t value = old_fread(ptr, size, nmemb, stream); dlclose(handle);

            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);
            printf("[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", (char *) ptr, size, nmemb, filename, value);

            return value;
        }

        size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_fwrite = (size_t(*)(const void *, size_t, size_t, FILE *)) dlsym(handle, "fwrite");
            size_t value = old_fwrite(ptr, size, nmemb, stream); dlclose(handle);

            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, 0, stream);
            printf("[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", (const char *) ptr, size, nmemb, filename, value);

            return value;
        }

        int open(const char *pathname, int flags, ...) {
            va_list vl; va_start(vl, flags); mode_t mode = va_arg(vl, mode_t);

            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_open = (int(*)(const char *, int, ...)) dlsym(handle, "open");
            int value = old_open(pathname, flags, mode); dlclose(handle);

            char num1[SIZE]; memset(num1, 0, sizeof(num1)); change(mode, num1);
            char num2[SIZE]; memset(num2, 0, sizeof(num2)); change(mode, num2);

            printf("[logger] open(\"%s\", %s, %s) = %d\n", pathname, num1, num2, value);

            return value;
        }

        ssize_t read(int fd, void *buf, size_t count) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_read = (ssize_t(*)(int, void *, size_t)) dlsym(handle, "read");
            ssize_t value = old_read(fd, buf, count); dlclose(handle);

            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);
            printf("[logger] read(\"%s\", \"%s\", %ld) = %ld\n", filename, (char *) buf, count, value);

            return value;
        }

        int remove(const char *pathname) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_remove = (int(*)(const char *)) dlsym(handle, "remove");
            int value = old_remove(pathname); dlclose(handle);

            printf("[logger] remove(\"%s\") = %d\n", pathname, value);

            return value;
        }

        int rename(const char *oldpath, const char *newpath){
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_rename = (int(*)(const char *, const char *)) dlsym(handle, "rename");
            int value = old_rename(oldpath, newpath); dlclose(handle);

            printf("[logger] rename(\"%s\", \"%s\") = %d\n", oldpath, newpath, value);

            return value;
        }

        FILE *tmpfile(void) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_tmpfile = (FILE *(*)(void)) dlsym(handle, "tmpfile");
            FILE *fp = old_tmpfile(); dlclose(handle);

            printf("[logger] tmpfile() = %p\n", fp);

            return fp;
        }

        ssize_t write(int fd, const void *buf, size_t count) {
            void *handle = dlopen("libc.so.6", RTLD_LAZY);

            old_write = (ssize_t(*)(int, const void *, size_t)) dlsym(handle, "write");
            ssize_t value = old_write(fd, buf, count); dlclose(handle);

            char path[PATH_MAX], filename[PATH_MAX]; get_filename(filename, path, fd, NULL);
            printf("[logger] write(\"%s\", \"%s\", %ld) = %ld\n", filename, (char *) buf, count, value);

            return value;
        }
    }
