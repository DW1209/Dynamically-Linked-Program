#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/stdc++.h>

#define MODE    0644
#define FLAGS   O_CREAT | O_RDWR | O_TRUNC

using namespace std;

void print_usage(void) {
    fprintf(stdout, "usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]\n");
    fprintf(stdout, "       -p: set the path to logger.so, default = ./logger.so\n");
    fprintf(stdout, "       -o: print output to file, print to \"stderr\" if no file specified\n");
    fprintf(stdout, "       --: separate the arguments for logger and for the command\n");
}

int main(int argc, char *argv[]) {
    int opt; pid_t pid; bool error = false;

// Set LD_PRELOAD = ./logger.so file.
    setenv("LD_PRELOAD", "./logger.so", 1);  

// Use getopt function to get option commands and set up.
    while ((opt = getopt(argc, argv, "o:p:")) != -1) {
        switch (opt) {
            case 'o': setenv("FILE", to_string(open(optarg, FLAGS, MODE)).c_str(), 1);  break;
            case 'p': setenv("LD_PRELOAD", optarg, 1);                                  break;
            default : error = true;                                                     break;
        }
    }

// Print out usage if there is invalid argument option.
    if (error) {
        print_usage(); exit(-1);
    }

// Do fork and exec if there is command given.
    if (optind < argc) {
        if ((pid = fork()) < 0) {
            perror("fork error"); exit(-1);
        } else if (pid == 0) {
            int new_fd = dup(fileno(stderr)); setenv("BACKUP", to_string(new_fd).c_str(), 1);

            if (execvp(argv[optind], argv + optind) < 0) {
                fprintf(stdout, "command not found: %s\n", argv[optind]); exit(-1);
            }

            exit(-1);
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        fprintf(stdout, "no command given.\n"); exit(-1);
    }

    return 0;
}
