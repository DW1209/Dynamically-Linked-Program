#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <bits/stdc++.h>

#define MAX 128

using namespace std;

void print_usage(void) {
    printf("usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]\n");
    printf("\t-p: set the path to logger.so, default = ./logger.so\n");
    printf("\t-o: print output to file, print to \"stderr\" if no file specified\n");
    printf("\t--: separate the arguments for logger and for the command\n");
}

int main(int argc, char *argv[]) {
    int opt; pid_t pid; map<char, string> records; 
    bool mark = false, leave = false;

// Allocate memories for commands variable.
    char **commands = (char**) malloc(MAX * sizeof(char*));
    for (int i = 0; i < MAX; i++) {
        commands[i] = (char*) malloc(MAX * sizeof(char));
    }

// Set LD_PRELOAD = ./logger.so file.
    setenv("LD_PRELOAD", "./logger.so", 1);  

// Use getopt function to get option commands.
    while ((opt = getopt(argc, argv, ":o:p:-")) != -1) {
        switch (opt) {
            case 'o': records['o'] = string(optarg); break;
            case 'p': records['p'] = string(optarg); break;
            case '?': print_usage();                 exit(-1);
            case '-': leave = true;                  break;
            default :                                break;
        }

        if (leave) break;
    }

// Check if there are -o and -p argument needed to be set.
    map<char, string>::iterator it = records.begin();
    for (; it != records.end(); it++) {
        switch (it->first) {
            case 'o': setenv("FILE", to_string(open(it->second.c_str(), O_CREAT, 0664)).c_str(), 1); break;
            case 'p': setenv("LD_PRELOAD", it->second.c_str(), 1);                                   break;
            default :                                                                                break;
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
            if (execvp(commands[0], commands) < 0) {
                printf("command not found: %s\n", commands[0]); exit(-1);
            }
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        cout << "no command given." << endl; exit(-1);
    }

    return 0;
}
