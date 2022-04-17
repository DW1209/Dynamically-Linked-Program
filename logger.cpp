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

#define SIZE    4
#define LIMIT   32

using namespace std;

extern "C" {
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

// Get FILE environment variable and change to file descriptor number.
    int get_FILE() {
        char *output_file = getenv("FILE"); 
        return (output_file != NULL)? strtol(output_file, NULL, 10): -1;
    }

// Get BACKUP environment variable and open the file.
    FILE* get_BACKUP() {
        char *word = getenv("BACKUP");
        return fdopen(strtol(word, NULL, 10), "w");
    }

// According to pathname, fd or FILE to get the filename by realpath or readlink function.
    int get_filename(char *filename, const char *pathname, int fd, FILE *stream) {
        char path[PATH_MAX]; memset(path, 0, PATH_MAX); memset(filename, 0, PATH_MAX);

        if (pathname != NULL) {
            if (realpath(pathname, filename) == NULL) {
                return -1;
            }
        } else {
            if (fd >= 0) {
                snprintf(path, PATH_MAX, "/proc/self/fd/%d", fd);
            }

            if (stream != NULL) {
                snprintf(path, PATH_MAX, "/proc/self/fd/%d", fileno(stream));
            }

            if (readlink(path, filename, PATH_MAX) < 0) {
                return -1;
            }
        }

        return 0;
    }

// Change not printable character to a dot, and limit the string to 32.
    void get_string(const void *buf, char *str, size_t count) {
        size_t total = (count < LIMIT)? count: LIMIT;
        memset(str, 0, LIMIT + 1); strncpy(str, (const char *) buf, total);

        for (size_t i = 0; i < total; i++) {
            if (!isprint(str[i])) str[i] = '.';
        }

        if (total == LIMIT) str[LIMIT] = 0;
    }

// Below are the monitored library calls.
    int chmod(const char *pathname, mode_t mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_chmod = (int(*)(const char *, mode_t)) dlsym(handle, "chmod");
        int value = old_chmod(pathname, mode); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] chmod(\"%s\", %o) = %d\n", filename, mode, value) < 0) {
                perror("chmod fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] chmod(\"%s\", %o) = %d\n", filename, mode, value) < 0) {
                perror("chmod dprintf error");
            }
        }

        return value;
    }

    int chown(const char *pathname, uid_t owner, gid_t group) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_chown = (int(*)(const char *, uid_t, gid_t)) dlsym(handle, "chown");
        int value = old_chown(pathname, owner, group); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] chown(\"%s\", %d, %d) = %d\n", filename, owner, group, value) < 0) {
                perror("chown fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] chown(\"%s\", %d, %d) = %d\n", filename, owner, group, value) < 0) {
                perror("chown dprintf error");
            }
        }

        return value;
    }

    int close(int fd) {
        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, NULL, fd, NULL);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_close = (int(*)(int)) dlsym(handle, "close");
        int value = old_close(fd); dlclose(handle);
        
        if (file == -1) {
            if (fprintf(backup, "[logger] close(\"%s\") = %d\n", filename, value) < 0) {
                perror("close fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] close(\"%s\") = %d\n", filename, value) < 0) {
                perror("close dprintf error");
            }
        }

        return value;
    }

    int creat(const char *pathname, mode_t mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_creat = (int(*)(const char *, mode_t)) dlsym(handle, "creat");
        int value = old_creat(pathname, mode); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] creat(\"%s\", %03o) = %d\n", filename, mode, value) < 0) {
                perror("creat fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] creat(\"%s\", %03o) = %d\n", filename, mode, value) < 0) {
                perror("creat dprintf error");
            }
        }

        return value;
    }

    int fclose(FILE *stream) {
        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, NULL, -1, stream);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fclose = (int(*)(FILE *)) dlsym(handle, "fclose");
        int value = old_fclose(stream); dlclose(handle);

        if (file == -1) {
            if (fprintf(backup, "[logger] fclose(\"%s\") = %d\n", filename, value) < 0) {
                perror("fclose fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] fclose(\"%s\") = %d\n", filename, value) < 0) {
                perror("fclose dprintf error");
            }
        }

        return value;
    }

    FILE *fopen(const char *pathname, const char *mode) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fopen = (FILE*(*)(const char *, const char *)) dlsym(handle, "fopen");
        FILE *fp = old_fopen(pathname, mode); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] fopen(\"%s\", \"%s\") = %p\n", filename, mode, fp) < 0) {
                perror("fopen fprintf error");
            }
        } else {            
            if (dprintf(file, "[logger] fopen(\"%s\", \"%s\") = %p\n", filename, mode, fp) < 0) {
                perror("fopen dprintf error");
            }
        }

        return fp;
    }

    size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fread = (size_t(*)(void *, size_t, size_t, FILE *)) dlsym(handle, "fread");
        size_t value = old_fread(ptr, size, nmemb, stream); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char str[LIMIT + 1]; get_string(ptr, str, value);
        char filename[PATH_MAX]; get_filename(filename, NULL, -1, stream);

        if (file == -1) {
            if (fprintf(backup, "[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", str, size, nmemb, filename, value) < 0) {
                perror("fread fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", str, size, nmemb, filename, value) < 0) {
                perror("fread dprintf error");
            }
        }

        return value;
    }

    size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_fwrite = (size_t(*)(const void *, size_t, size_t, FILE *)) dlsym(handle, "fwrite");
        size_t value = old_fwrite(ptr, size, nmemb, stream); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char str[LIMIT + 1]; get_string(ptr, str, value);
        char filename[PATH_MAX]; get_filename(filename, NULL, -1, stream);

        if (file == -1) {
            if (fprintf(backup, "[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", str, size, nmemb, filename, value) < 0) {
                perror("fwrite fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", str, size, nmemb, filename, value) < 0) {
                perror("fwrite dprintf error");
            }
        }

        return value;
    }

    int open(const char *pathname, int flags, ...) {
// Check whether flags include O_CREAT or not and use va_arg function to get the additional argument.
        va_list vl; va_start(vl, flags); mode_t mode = (flags & O_CREAT)? va_arg(vl, mode_t): 0;

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_open = (int(*)(const char *, int, ...)) dlsym(handle, "open");
        int value = old_open(pathname, flags, mode); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] open(\"%s\", %03o, %03o) = %d\n", filename, flags, mode, value) < 0) {
                perror("open fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] open(\"%s\", %03o, %03o) = %d\n", filename, flags, mode, value) < 0) {
                perror("open dprintf error");
            }
        }

        return value;
    }

    ssize_t read(int fd, void *buf, size_t count) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_read = (ssize_t(*)(int, void *, size_t)) dlsym(handle, "read");
        ssize_t value = old_read(fd, buf, count); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char str[LIMIT + 1]; get_string(buf, str, value);
        char filename[PATH_MAX]; get_filename(filename, NULL, fd, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] read(\"%s\", \"%s\", %ld) = %ld\n", filename, str, count, value) < 0) {
                perror("read fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] read(\"%s\", \"%s\", %ld) = %ld\n", filename, str, count, value) < 0) {
                perror("read dprintf error");
            }
        }

        return value;
    }

    int remove(const char *pathname) {
        int file = get_FILE(); FILE *backup = get_BACKUP();
        char filename[PATH_MAX]; get_filename(filename, pathname, -1, NULL);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_remove = (int(*)(const char *)) dlsym(handle, "remove");
        int value = old_remove(pathname); dlclose(handle);

        if (file == -1) {
            if (fprintf(backup, "[logger] remove(\"%s\") = %d\n", filename, value) < 0) {
                perror("remove fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] remove(\"%s\") = %d\n", filename, value) < 0) {
                perror("remove dprintf error");
            }
        }

        return value;
    }

    int rename(const char *oldpath, const char *newpath){
        int file = get_FILE(); FILE *backup = get_BACKUP();
        char old_filename[PATH_MAX]; get_filename(old_filename, oldpath, -1, NULL);
        char new_filename[PATH_MAX]; get_filename(new_filename, newpath, -1, NULL);

        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_rename = (int(*)(const char *, const char *)) dlsym(handle, "rename");
        int value = old_rename(oldpath, newpath); dlclose(handle);

        if (file == -1) {
            if (fprintf(backup, "[logger] rename(\"%s\", \"%s\") = %d\n", old_filename, new_filename, value) < 0) {
                perror("rename fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] rename(\"%s\", \"%s\") = %d\n", old_filename, new_filename, value) < 0) {
                perror("rename dprintf error");
            }
        }

        return value;
    }

    FILE *tmpfile(void) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_tmpfile = (FILE *(*)(void)) dlsym(handle, "tmpfile");
        FILE *fp = old_tmpfile(); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        
        if (file == -1) {
            if (fprintf(backup, "[logger] tmpfile() = %p\n", fp) < 0) {
                perror("tmpfile fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] tmpfile() = %p\n", fp) < 0) {
                perror("tmpfile dprintf error");
            }
        }

        return fp;
    }

    ssize_t write(int fd, const void *buf, size_t count) {
        void *handle = dlopen("libc.so.6", RTLD_LAZY);

        old_write = (ssize_t(*)(int, const void *, size_t)) dlsym(handle, "write");
        ssize_t value = old_write(fd, buf, count); dlclose(handle);

        int file = get_FILE(); FILE *backup = get_BACKUP();
        char str[LIMIT + 1]; get_string(buf, str, count);
        char filename[PATH_MAX]; get_filename(filename, NULL, fd, NULL);

        if (file == -1) {
            if (fprintf(backup, "[logger] write(\"%s\", \"%s\", %ld) = %ld\n", filename, str, value, value) < 0) {
                perror("write fprintf error");
            }
        } else {
            if (dprintf(file, "[logger] write(\"%s\", \"%s\", %ld) = %ld\n", filename, str, value, value) < 0) {
                perror("write dprintf error");
            }
        }

        return value;
    }
}
