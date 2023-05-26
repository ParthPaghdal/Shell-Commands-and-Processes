# Shell-Commands-and-Processes

## Overview

In this project, I have developed a simple Linux shell. The shell accepts user commands and then
executes each command in a separate process. The shell provides the user a prompt at which the next
command is entered. One technique for implementing a shell interface is to have the parent process
first read what the user enters on the command line and then create a separate child process that
performs the command. Unless otherwise specified, the parent process waits for the child to exit
before continuing. However, UNIX shells typically also allow the child process to run in the
background or concurrently by specifying the ampersand (`&`) at the end of the command. The separate
child process is created using the `fork()` system call and the user's command is executed by using
one of the system calls in the `exec()` family.

## A Simple Shell

A C program that provides the basic operation of a command line shell is given below. The `main()`
function first calls `read_command()`, which reads a full command from the user and tokenizes it
into separate words (arguments). These tokens can be passed directly to `execvp()` in the child
process. If the user enters an `&` as the final argument, `read_command()` will set the
`in_background` parameter to true (and remove the `&` from the array of tokens). 


## Problems

There are four problems that needed to solve:  

* Create the child process and executing the command in the child.
* Implement some internal commands.
* Implement a history feature.
* Better `cd` Command

### Problem 1. Creating Child Process

First, modify `main()` so that upon returning form `read_command()`, a child is forked and executes
the command specified by the user. As noted above, `read_command()` loads the contents of the
`tokens` array with the command specified by the user. This `tokens` array will be passed to the
`execvp()` function.

**Note:** When a process in Linux finishes, it still exists in the kernel with some status
information until the parent process waits on that child. These un-waited-on terminated child
processes are known zombies. 

### Problem 2. Shell Prompt and Internal Commands

Implemented some internal commands. Internal commands are built-in features of the shell itself,
as opposed to a separate program that is executed. Note that for these you need not fork a new process as they can be handled directly in the parent process. All
the commands here are **case-sensitive**.

* `exit`: Exit the shell program. If the user provided any argument, abort the operation (i.e.,
  command not executed) and display an error message.
* `pwd`: Display the current working directory. Use the `getcwd()` function. Run `man getcwd` for
  more. Again, abort the operation and display an error message if the user provided any argument.
* `cd`: Change the current working directory. Use the `chdir()` function. Pass `chdir()` the first
  argument the user enters (it will accept absolute and relative paths). If the user passed in more
  than one argument, abort the operation and display an error message. If `chdir()` returns an
  error, display an error message.
* `help`: Display help information on internal commands.
    * If the first argument is one of our internal commands, print `<argument> is a builtin command`
      plus a brief description on what the command does. For example, if argument is `cd`, the
      output should be:
        
        ```
        'cd' is a builtin command for changing the current working directory.
        ```
        
    * If the first argument is not an internal command, this command prints `<argument> is an
      external command or application`. For example, if argument is `ls`, the output must be:
        
        ```
        'ls' is an external command or application
        ```
        
    * If there is more than one argument, display an error message.
    * If there is no argument provided, list all the supported internal commands. For each command,
      include a short summary on what it does.

### Problem 3. History Feature

The next task is to modify shell to provide a history feature that allows the user access up to
the 10 most recently entered commands. Start numbering the user's commands at 0 and increment for
each command entered. These numbers will grow past 9. For example, if the user has entered 35
commands, then the most recent 10 will be numbered 15 through 34.

**History Commands**: First implement an internal command `history` which displays the 10 most
recent commands executed in the shell. If there are less than 10 commands entered, display all the
commands entered so far.

* Display the commands in descending order (sorted by its command number).
* The output should include both external application commands and internal commands.
* Display the command number on the left, and the command (with all its arguments) on the right.
  * Hint: Print a tab between the two outputs to have them line up easily.
  * If the command is run in the background using `&`, it must be added to the history with the `&`.
* A sample output of the history command is shown below:

```
/home/cmpt300$ history
30	history
29	cd /home/cmpt300
28	cd /proc
27	cat uptime
26	cd /usr
25	ls
24	man pthread_create
23	ls
22	echo "Hello World from my shell!"
21	ls -la
/home/cmpt300$
```

Next, implement the `!` and related commands which allows users to run commands directly from the
history list:

* Command `!n` runs command number n, such as `!22` will re-run the 23rd command entered this
  session. In the above example, this will re-run the echo command.
    * If n is not a number, or an invalid value (not one of the previous 10 command numbers) then
      display an error.
    * You may treat any command starting with `!` as a history command. For example, if the user
      types `!hi`, just display an error. Note that `atoi("hi")` returns 0, which should not be
      treated as a valid command.
* Command `!!` runs the previous command.
    * If there is no previous command, display an error message.
* When running a command using `!n` or `!!`, display the command from history to the screen so the
  user can see what command they are actually running.
* Neither the `!!` nor the `!n` commands are to be added to the history list themselves, but rather
  the command being executed from the history must be added. Here is an example.

```
/home/cmpt300$ echo test
test
/home/cmpt300$ !!
echo test
test
/home/cmpt300$ history
15	history
14	echo test
13	echo test
12	ls
11	man pthread_create
10	cd /home/cmpt300
9	ls
8	ls -la
7	echo Hello World from my shell!
6	history
/home/cmpt300$
```

**Signals:** Change your shell program to display the help information when the user presses
`ctrl-c` (which is the `SIGINT` signal).

* In `main()`, register a custom signal handler for the `SIGINT` signal.
* Have the signal handler display the help information (same as the `help` command).
* Then re-display the command-prompt before returning.
* Note that when another child process is running, `ctrl-c` will likely cancel/kill that process.
  Therefore displaying the help information with `ctrl-c` will only be tested when there are no
  child processes running.

### Problem 4 Better `cd` Command

Implemented the following features that are often supported by the `cd` command in modern shells
(e.g., bash).

* Change to the home directory if no argument is provided.
* Support `~` for the home folder. For example, `cd ~/cmpt300` should change to the "cmpt300"
  directory under the current user's home directory. Issuing `cd ~` will switch to the home
  directory.
* Support `-` for changing back to the previous directory. For example, suppose that the current
  working directory is `/home` and you issued `cd /` to change to the root directory. Then, `cd -`
  will switch back to the `/home` directory.
