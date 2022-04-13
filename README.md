# Monitor File Activities of Dynamically Linked Programs
## Introduction
In this program, it is aim to practice library injection and API hijacking. Implementing a simple logger program that can show **file-access-related** activities of an arbitrary binary running on a Linux operating system. The program will implement the logger in two parts. One is a **logger** program that prepares the runtime environment to inject, load, and execute a monitored binary program. The other is a **shared object** that can be injected into a program by the **logger** using **LD_PRELOAD**. 

## Program Arguments
The program will work with the following arguments. If an invalid argument is passed to the **logger**, the below message will be displayed.
```console
usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]
    -p: set the path to logger.so, default = ./logger.so
    -o: print output to file, print to "stderr" if no file specified
    --: separate the arguments for logger and for the command
``` 

## Execution
```console
$ make
$ ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]
```

## Explanation
* The monitored library calls are **chmod**, **chown**, **close**, **creat**, **fclose**, **fread**, **fwrite**, **open**, **read**, **remove**, **rename**, **tmpfile**, **write**.
*  If a passed argument is a filename string, it will print the **real absolute path** of the file by using **`realpath(3)`**. If **`realpath(3)`** cannot resolve the filename string, the program will simply print out the string untouched.
* If a passed argument is a descriptor or a FILE * pointer, it will print the **absolute path** of the corresponding file. The filename for a corresponding descriptor can be found in **`/proc/{pid}/fd`** directory.
* If a passed argument is a mode or a flag, it will print out the value in octal.
* If a passed argument is an integer, it will simply print out the value in decimal.
* If a passed argument is a regular character buffer, it will print it out up to 32 bytes. The program checks each output character using **`isprint(3)`** function and output a dot '.' if a character is not printable.
* If a return value is an integer, it will simply print out the value in decimal.
* If a return value is a pointer, it will print out the pointer by using **`%p`** format conversion specifier.
* The output strings would be quoted with double quotes.