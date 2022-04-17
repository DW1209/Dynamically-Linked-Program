#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/stdc++.h>

#define MAX     128
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
    bool mark = false, error = false;
    int opt; pid_t pid; map<char, string> records; 

// Allocate memories for commands variable.
    char **commands = (char**) malloc(MAX * sizeof(char*));
    for (int i = 0; i < MAX; i++) {
        commands[i] = (char*) malloc(MAX * sizeof(char));
    }

// Set LD_PRELOAD = ./logger.so file.
    setenv("LD_PRELOAD", "./logger.so", 1);  

// Use getopt function to get option commands.
    while ((opt = getopt(argc, argv, "o:p:")) != -1) {
        switch (opt) {
            case 'o': records['o'] = string(optarg); break;
            case 'p': records['p'] = string(optarg); break;
            default : error = true;                  break;
        }
    }

    if (error) {
        print_usage(); exit(-1);
    }

// Check if there are -o and -p argument needed to be set.
    map<char, string>::iterator it = records.begin();
    for (; it != records.end(); it++) {
        switch (it->first) {
            case 'o': setenv("FILE", to_string(open(it->second.c_str(), FLAGS, MODE)).c_str(), 1); break;
            case 'p': setenv("LD_PRELOAD", it->second.c_str(), 1);                                 break;
            default :                                                                              break;
        }
    }

// Get command and store them inside the commands variable.
    for (int index = optind, i = 0; index < argc; index++, i++) {
        commands[i] = argv[index]; mark = true;
        if (index == argc - 1) commands[i + 1] = NULL;
    }

// Do fork and exec if there is command given.
    if (mark) {
        if ((pid = fork()) < 0) {
            perror("fork error"); exit(-1);
        } else if (pid == 0) {
            int new_fd = dup(fileno(stderr)); setenv("BACKUP", to_string(new_fd).c_str(), 1);

            if (execvp(commands[0], commands) < 0) {
                fprintf(stdout, "command not found: %s\n", commands[0]); exit(-1);
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
