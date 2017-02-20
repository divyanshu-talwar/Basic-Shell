# Advanced C Module Lab - 2
## About
* A shell which executes the commands listed in the directories `/bin` or `/usr/bin` using the system calls `fork`, `execv` and `wait`.
* The shell exits if the `getline` function returns an invalid string or exit.
* Then the entered string is delimited by `" "`.
* The execv command searches for the command entered and runs it in the child process.
* `Command not found!` is printed if no such command exists.
* Contributor - Divyanshu Talwar (2015028)

## Installation and Execution
* Run `make` command on the console and the Makefile would generate an object file `ush`
* Run the object file and the shell executes.
* STD output will be generated.

-------------------


Develop a basic shell. Your shell should launch in a separate window. The shell should also be able to handle CTRL-C elegantly i.e. it should not result in killing the shell. 

Functionalities expected:

cd, history, clear, piping, redirecting stdin and stdout (using > and <), kill, handling Cntrl+C, handling erroneous commands, handling return (enter), help which will list all the built-in commands implemented by your shell.
Use of fork, exec, wait etc.

No use of 'system' command - you have to write your own shell and not use existing shell.

Submit Guidelines:
Submit You program file by naming it: ROLLNO_Shell1.c 

ROLLNO should be written as 2015xxx.
More than one file should be submitted by adding a suffix of _x to the name, e.g.
ROLLNO_Shell_1.c, ROLLNO_Shell_2.c