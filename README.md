# Operating Systems Assignment - 2
## About
* This is a basic shell implemented in C.
* Contributor - Divyanshu Talwar (2015028)

### Basic Functionalities
* The shell does not exit on `Ctrl + C`. 
* Built in functions - `cd` , `history`, `exit`, `help`.
* The shell's history is maintain in the form of a queue and is also written into `.ush_history`, thus the shell remembers the commands, even if the shell is restarted.
* The previous commands can be accessed by pressing the `up arrow` and the next by pressing the `down arrow` (implemented with the help of history in `readline` function).
* The shell can run all the commands listed in the directories `/bin` or `/usr/bin`.
* The shell can handle piping `|`, redirection of stdin and/or stdout `> and/or <` .
* It can `kill` only those processes that are started by it, if shell is not started in root mode (by default shell is not started in root mode ), else it can kill any process except `init`.
* The `help` command lists all the built in commands.
* An erroneous command results in printing of `Command not found!` on stdout.
* The shell exits if an `EOF character` or `exit` is typed on the console.
* Typing `enter` on `stdin` takes the prompt to the next line.

## Installation and Execution
* Run `make` command on the console and the Makefile would run the shell.