-SHELL-
 Authored by Shiran Glasser
 208608174

==Description==
The program builds a shell using C language at the operating system linux.   At first, it builds the prompt line that will be precented each iteration for the user. It finds the programs username (using geteuid(), declared in: pwd.h), and the current path (using getcwd(), declared in: unistd.h).
The program shows the user the prompt line with the details,  the user enters commands until the word "done" is entered.  It checks if a pipe was entered(the symbol |).
The program counts how many pipes were entered and acts according to one of the following cases:
-If there aren’t any pipes- executes the entered command by orders .
when ^z is entered, the child that currently running the command' will stop (while the main process still runs and gets other commands), the process stopes until the user entered 'fg'.
after the child terminated completely,  the parent process will free the used memory.
-if there is one pipe- built a pipe, execute the first command and assigning the output to the pipe, then execute the second command using the input from the pipe.
-if there are two pipes-  built 2 pipes. The first to execute the first 2 commands like before, and the second to assign the second commands input to the pipe' where the third command will get its input from and execute to the screen. 
-if there are more then two pipes- continue to the next command.
-The command will be executed by the follow:
 The program transforms the entered command to a two-dimensional array format, and send it to execution. The array contains sells as the number of words in the command, in each sell there's a word and at the last one NULL (as the standard format for the executing commands system call). 
-The program make a new process (using fork() ), the child(new) process executes the given command (using execvp() ) while the main  process waits for the command to finish executing and continue to get the next command.
-The program also counts the number of all commandes entered, the number of pipes and prints this details at the end.
An edge case that the program handle is when the command "cd" entered, at the case it doesn’t handle this command.
if the entered command was not found by the system, a massage will appear.
The program uses methods and code parts from exercise 1, exercise 2, exercise 3, and the following methods:  

==functions: ==
Help methods for handle no pipes for the implementation of the signals.
- only the parent, the main process call this methods. : (2 added functions)
1. sig_ignore - changes the default implematation of the ^z signal (SIGTSTP). When the parents gets this input, he goes to this function and ignore it.  
2. sig_handler- handle the SIGCHILD that the parent process gets every time the child process changes its status(stops/continue, terminates), the method has waitpid inside that orders the process to continue every time the child changes its status and didn’t terminated.
3. handleNoPipe- receives the input command, transform it into an char** array which contain in each sell a separated word of the command.
   creates a son process that will execute the command.
   if the word 'done' was entered, returns -1 so the main will no to stop.
   supports the signals ^z ang fg to stop and resume a running process
   receives also a pointer to the last stopped child process's id to resume it when fg enters.

 .4 . handle1Pipe-   receives the full input string and the location of the pipe in it.
   creates 2 sons processes that will execute the 2 commands by order

.  5.  handle2Pipes- receives the full input string ang the location of the 2 pipes in it.
   creates 3 sons processes that will execute the 3 commands by order
6. getPrompt- get the current username and path and make the prompt format.

7. howManyWords- a function from ex1 that counts how many words are in the sentence.
    with an addition for comands with spaces in a block of " ".

8.      getWord-('printWord' in ex1)- gets the string of the sentence and its length returns the next word from index i(after spaces)
           updates i (by reference) to be at the next after. I changed the method from ex2 that it will not use the method 'Substr' and will find the substring directly in this method. 
         with an addition for comands with spaces in a block of " ".                                                                                                                                    

9.  buildArray- the method gets the string input from the user, and an empty char ** array and size-the amount of words in the command.
 the method separates the input sentence into words (using the code from ex1), and assigning each word into a sell in the array
 at the last sells' enters NULL for the command and \0 for the string
 
10.  freeArray-  free the assigned memory after each command. first free each word's memory (gets the amount of words in int), at the end frees the array.

11. (a new method:) wherePipe- returns the index of the pipe in the received string(starting from the given index), if there's no pipe(reached the end of the string), returns -1


 
Program DATABASE:
* An 2D array assigning dynamically for each command as the size of the number of words. 
* Pipes for communication between processes. 

==How to compile?==
gcc shell.c -o shell
run: ./shell

==Input:==
The user enters commands of his chois.(an input sentence that has more than 510 chars, with or without | (=pipes))                        
The program stopes at the input "done".
The input 'cd' is not supported yet by the program. (also before and after pipes) 
An input of more that 2 pipes will not be supported by the program.
The program assumes that the input doesnt contain an empty command.
When the user enters ^z the program stops until he enters fg.
==Output:==
*An prombt line contains the current username and id in this format:  user@currentdir>

*The execution of all the commands that were entered.
if there was a pipe: the output of the first command will be the input of the second command (that will be the input of the third, if entered), and the output of the last command will appear on screen.
  
    if the command is 'cd' – this massage: "command not supported (yet)" 
    If the command is not supported by the system- "command was not found"
    If there's more than 2 pipes- "more than 2 pipes are not allowed"

*after the word 'done' and exiting the program, will be an output of: 
   1. the total number of commands were entered(without including 'done').
   2. the total number of pipes in were entered.
   3. the massage: "See you next time !".