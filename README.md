# MyShell
Implement a command line interpreter, or shell, on top of Linux

The shell functions like this : When you type in a command (in response to its prompt), the shell creates a child process that executes the command you entered and then prompts for more user input when it has finished.  More specifically, shells are typically implemented as a simple loop that waits for input and fork()s a new child process to execute the command; the child process then exec()s the specified command while the parent process wait()s for the child to finish before continuing with the next iteration of the loop.

The shell implemented here will be similar to, but much simpler, than the one that we run every day in Unix. Besides the most basic function of executing commands, this shell (mysh) provides the following three features: interactive vs. batch mode, output redirection and aliasing. This project is done as part of Operating System coursework.
