/*

 Authored by Shiran Glasser
 208608174
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LEN 512 //same as the limit in ex1 code

//help methods:

//get the current username and path and make the prompt format
void getPrompt(char *);

//a function from ex1 that counts how many words are in the sentence
int howManyWords(char *, int);

/*'printWord' in ex1-
 gets the string of the sentence and its length
returns the next word from index i(after spaces)
updates i (by reference) to be at the next after.
*/
char *getWord(char *,int, int* i);

/* the method gets the string input from the user, and an empty char ** array and size-the amount of words in the command.
 the method separates the input sentence into words (using the code from ex1), and assigning each word into a sell in the array
 at the last sells' enters NULL for the command and \0 for the string
*/
void buildArray(char **, char*, int);

//free the assigned memory after each command
//first free each word's memory (gets the amount of words in int), at the end frees the array.
void freeArray(char**, int);

//returns the index of the pipe in the received string(starting from the given index), if theres no pipe(reached thr end of the string), returns -1
int wherePipe(char*, int);

/*
   receives the input command, transform it into an char** array which contain in each sell a separated word of the command.
   creates a son process that will execute the command.
   if the word 'done' was entered, returns -1 so the main will no to stop.
   supports the signals ^z ang fg to stop and resume a running process
   receives also a pointer to the last stopped child process's id to resume it when fg enters.
 */
int handleNoPipe(char*, int *);

/*
  receives the full input string and the location of the pipe in it.
   creates 2 sons processes that will execute the 2 commands by order
 */
void handle1Pipe(char*, int);

/*receives the full input string ang the location of the 2 pipes in it.
   creates 3 sons processes that will execute the 3 commands by order  */
void handle2Pipes(char*, int, int);


//handle the SIGCHILD that the parent process gets every time the child process changes its status(stops/continue, terminates),
// the method has waitpid inside that orders the process to continue every time the child changes its status and didn’t terminated.
void sig_handler(int s)
{
    waitpid(-1, NULL, WNOHANG);
}
//changes the default implematation of the ^z signal (SIGTSTP). When the parents gets this input, he goes to this function and ignore it.
void sig_ignore(int s)
{

}



int main() {
    char prompt[10 * sizeof(char) + PATH_MAX +3]; //the full line that will be printed each time' path +10 for username+ @, >, \0
    char inputStr[MAX_LEN]; //for the username's commands input
    int comCount = 0;
    int pipeCount = 0;
    int p1, p2; //the indexes of the pipes locations
    int pid=-1; //to save the last stopped process's id


    getPrompt(prompt);





    do {
        printf("%s", prompt);
        fgets(inputStr, MAX_LEN, stdin);
        if (inputStr == NULL) {
            perror("fgets error");
            exit(1);
        }

        comCount++;
        p1=wherePipe(inputStr, 0);
        if(p1 != -1) //theres one or more pipes
        {
            p2=wherePipe(inputStr, p1);
            if(p2!= -1) //there are 2 or more pipes
            {
                if(wherePipe(inputStr, p2) !=-1) //more than 2
                {
                    fprintf(stderr, "\nmore than 2 pipes are not allowed\n");
                    continue;
                }
                //else- exactly 2 pipes
                pipeCount+=2;
                handle2Pipes(inputStr, p1, p2);
            }
            else //exactly 1 pipe
            {
                pipeCount++;
                handle1Pipe(inputStr, p1);
                continue;
            }
        }
        else
        { //no pipes, regular case

            if (handleNoPipe(inputStr, &pid)== -1) //if the command is done
            {
                comCount--; //done doesnt count as a command
                break;
            }

        }

    }while (1);

    printf("Num of commands: %d\n", comCount);
    printf("Num of pipes: %d\n", pipeCount);
    printf("See you next time !\n");

    return 0;
}



/* 3 MAIN METHODS: */

/* (1)----------------------------------------------------*/
int handleNoPipe(char* inputStr, int* pid)
{
    char **comArr; //the current command
    int wordNum;
    signal(SIGCHLD, sig_handler); //the parent will continue after every interruption of the son and after he finishes frees sources
    signal(SIGTSTP, sig_ignore); //the parent should not follow ^z

    wordNum = howManyWords(inputStr, strlen(inputStr));
    if (wordNum == 0)
        return 1;

    comArr = (char **) malloc(sizeof(char *) * (wordNum + 2)); //num of words+ NULL+ \0
    if (comArr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        exit(1);
    }
    buildArray(comArr, inputStr, wordNum);

    //exiting method
    if ((strcmp(comArr[0], "done") == 0) && (wordNum == 1)) {
        freeArray(comArr, wordNum+2);
        return -1;
    }


    //edge case, the program doesn't support cd command
    if (strcmp(comArr[0], "cd") == 0) {
        puts("command not supported (yet)");
        return 1;
    }

    //resume a stopped process
    if ((strcmp(comArr[0], "fg") == 0) && (wordNum == 1))
    {
        if(*pid!=-1) //if NULL- no process has stopped, do nothing
        {
            kill(*pid, SIGCONT);   //resumes the stopped process
            *pid=-1;
        }
        return 1;
    }

    //creating process and executing
    pid_t child;
    child = fork(); //creating a child process that will execute the commend while the parend waits.
    if (child == -1) {
        perror("process creating failed");
        freeArray(comArr, wordNum + 2);
        exit(1);

    } else if (child == 0) //child
    {
        signal(SIGTSTP, SIG_DFL); //implements ^z as default
        if (execvp(comArr[0], comArr) < 0) {    // execute the command
            perror("command not found \n");
        }
        exit(0);
    }
    //parent
    int status;
    waitpid(child, &status, WUNTRACED);
    if(WIFSTOPPED(status))  //saves the last process that stopped in order to continue this one when fg entered
        *pid = child;

    freeArray(comArr, wordNum + 2);


    return 1;
}
/* ----------------------------------------------------*/




/* (2)----------------------------------------------------*/
void handle1Pipe(char* inputStr, int p)
{
    char **com1Arr; //the firs command
    char **com2Arr; //the second command
    char*firsCom;
    char*secondCom;
    int wordNum1;
    int wordNum2;


    //first command:
    firsCom=(char*) malloc((p+1)*sizeof(char)); //adding 1 to even with \n of regular input
    memset(firsCom, '\0', sizeof(firsCom));
    strncpy(firsCom, inputStr, p);  //building a substring for the first command
    firsCom[p]='\n';
    wordNum1 = howManyWords(firsCom, strlen(firsCom)+1);
    if (wordNum1 == 0)
        return;
    //creating the first command by the execution format
    com1Arr = (char **) malloc(sizeof(char *) * (wordNum1 + 2)); //num of words+ NULL+ \0
    if (com1Arr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        free(firsCom);
        exit(1);
    }
    buildArray(com1Arr, firsCom, wordNum1);
    free(firsCom);
    //edge case, the program doesn't support cd command
    if (strcmp(com1Arr[0], "cd") == 0) {
        puts("command not supported (yet)");
        freeArray(com1Arr, wordNum1+2);
        return;
    }


    //second command:
    secondCom=(char*) malloc((strlen(inputStr)-p+1)*sizeof(char));
    memset(secondCom, '\0', sizeof(secondCom));
    strncpy(secondCom, inputStr+p+1, strlen(inputStr)-p);  //building a substring for second command
    wordNum2= howManyWords(secondCom, strlen(secondCom)+1);
    if (wordNum2 == 0)
        return;
    //creating the second command by the execution format
    com2Arr = (char **) malloc(sizeof(char *) * (wordNum2 + 2)); //num of words+ NULL+ \0
    if (com2Arr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        freeArray(com1Arr, wordNum2+2);
        free(secondCom);
        exit(1);
    }
    buildArray(com2Arr, secondCom, wordNum2);
    free(secondCom);
    //edge case, the program doesn't support cd command
    if (strcmp(com2Arr[0], "cd") == 0) {
        puts("command not supported (yet)");
        freeArray(com1Arr, wordNum1+2);
        freeArray(com2Arr, wordNum2+2);
        return;
    }

    //creating the pipe
    int pipe_fd[2], val;
    if(pipe(pipe_fd)==-1)
    {
        perror("pipe failed\n");
        exit(1);
    }

    // creating the processes and connecting them with the pipe
    pid_t child1;
    pid_t child2;
    child1 = fork(); //creating the first child process that will execute the first commend
    if (child1 == -1) {
        perror("process creating failed");
        freeArray(com1Arr, wordNum1 + 2);
        freeArray(com2Arr, wordNum2 + 2);
        exit(1);
    }

    else if (child1 == 0) //child1
    {
        close(pipe_fd[0]);

        val = dup2(pipe_fd[1], STDOUT_FILENO); //changing its default output to the pipe
        if (val == -1) {
            freeArray(com1Arr, wordNum1 + 2);
            freeArray(com2Arr, wordNum2 + 2);
            close(pipe_fd[1]);
            fprintf(stderr, "dup2 failed1\n");
            exit(1);
        }

        //executing the first command
        if (execvp(com1Arr[0], com1Arr) < 0) {
            perror("command not found \n");
        }

        close(pipe_fd[1]);
        exit(0);

    } /*parent,
            after child1 executed the commend, create child 2 to execute the second command */

    child2 = fork(); //creating the second child process that will execute the second commend
    if (child2 == -1) {
        perror("process creating failed");
        freeArray(com1Arr, wordNum1 + 2);
        freeArray(com2Arr, wordNum2 + 2);
        exit(1);
    }

    else if (child2 == 0)
    {
        close(pipe_fd[1]);

        val = dup2(pipe_fd[0], STDIN_FILENO); //changing its default input to be from the pipe
        if (val == -1) {
            freeArray(com1Arr, wordNum1 + 2);
            freeArray(com2Arr, wordNum2 + 2);
            close(pipe_fd[0]);
            fprintf(stderr, "dup2 failed1\n");
            exit(1);
        }

        //executing the second command
        if (execvp(com2Arr[0], com2Arr) < 0) {
            perror("command not found \n");
        }
        close(pipe_fd[0]);
        exit(0);
    }

    // only the parent gets here and waits for 2 children to finish
    close(pipe_fd[0]);
    close(pipe_fd[1]);

    for (int i = 0; i < 2; ++i)
        wait(NULL);

    freeArray(com1Arr, wordNum1 + 2);
    freeArray(com2Arr, wordNum2 + 2);
    return;
}
/* ----------------------------------------------------*/



/* (3)----------------------------------------------------*/
void handle2Pipes(char* inputStr, int p1, int p2)
{
    char **com1Arr; //the firs command
    char **com2Arr; //the second command
    char **com3Arr; //the third command
    char*firsCom;
    char*secondCom;
    char*thirdCom;
    int wordNum1;
    int wordNum2;
    int wordNum3;


    //first command:
    firsCom=(char*) malloc((p1+1)*sizeof(char)); //adding 1 to even with \n of regular input
    memset(firsCom, '\0', sizeof(firsCom));
    strncpy(firsCom, inputStr, p1);  //building a substring for the first command
    firsCom[p1]='\n';
    wordNum1 = howManyWords(firsCom, strlen(firsCom));
    if (wordNum1 == 0)
        return;
    //creating the first command by the execution format
    com1Arr = (char **) malloc(sizeof(char *) * (wordNum1 + 2)); //num of words+ NULL+ \0
    if (com1Arr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        free(firsCom);
        exit(1);
    }
    buildArray(com1Arr, firsCom, wordNum1);
    free(firsCom);
    //edge case, the program doesn't support cd command
    if (strcmp(com1Arr[0], "cd") == 0) {
        puts("command not supported (yet)");
        freeArray(com1Arr, wordNum1+2);
        return;
    }


    //second command:
    secondCom=(char*) malloc((p2-p1+1)*sizeof(char));
    memset(secondCom, '\0', sizeof(secondCom));
    strncpy(secondCom, inputStr+p1+1, p2-p1);  //building a substring for second command
    wordNum2= howManyWords(secondCom, strlen(secondCom));
    if (wordNum2 == 0)
        return;
    //creating the second command by the execution format
    com2Arr = (char **) malloc(sizeof(char *) * (wordNum2 + 2)); //num of words+ NULL+ \0
    if (com2Arr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        freeArray(com1Arr, wordNum1+2);
        free(secondCom);
        exit(1);
    }
    buildArray(com2Arr, secondCom, wordNum2);
    free(secondCom);
    //edge case, the program doesn't support cd command
    if (strcmp(com2Arr[0], "cd") == 0) {
        puts("command not supported (yet)");
        freeArray(com1Arr, wordNum1+2);
        freeArray(com2Arr, wordNum2+2);
        return;
    }

    //third command:
    thirdCom=(char*) malloc((strlen(inputStr)-p2+1)*sizeof(char));
    memset(thirdCom, '\0', sizeof(thirdCom));
    strncpy(thirdCom, inputStr+p2+1, strlen(inputStr)-p2);  //building a substring for second command
    wordNum3= howManyWords(thirdCom, strlen(thirdCom));
    if (wordNum3 == 0)
        return;
    //creating the second command by the execution format
    com3Arr = (char **) malloc(sizeof(char *) * (wordNum3 + 2)); //num of words+ NULL+ \0
    if (com3Arr == NULL) {
        perror("There was a problem with the dynamic assigning\n");
        freeArray(com1Arr, wordNum1+2);
        freeArray(com2Arr,wordNum2+2);
        free(thirdCom);
        exit(1);
    }
    buildArray(com3Arr, thirdCom, wordNum3);
    free(thirdCom);
    //edge case, the program doesn't support cd command
    if (strcmp(com3Arr[0], "cd") == 0) {
        puts("command not supported (yet)");
        freeArray(com1Arr, wordNum1+2);
        freeArray(com2Arr, wordNum2+2);
        freeArray(com3Arr, wordNum3+2);
        return;
    }



    //creating the 2 pipes
    int pipe_fd1[2], pipe_fd2[2], val;
    if(pipe(pipe_fd1)==-1)
    {
        perror("pipe1 failed\n");
        exit(1);
    }
    if(pipe(pipe_fd2)==-1)
    {
        perror("pipe2 failed\n");
        exit(1);
    }

    // creating the processes and connecting them with the pipe
    pid_t child1;
    pid_t child2;
    pid_t child3;

    //creating the first child process that will execute the first commend
    child1 = fork();
    if (child1 == -1) {
        perror("process creating failed");
        freeArray(com1Arr, wordNum1 + 2);
        freeArray(com2Arr, wordNum2 + 2);
        freeArray(com3Arr, wordNum3+2);
        exit(1);
    }

        //CHILD (1)
    else if (child1 == 0)
    {
        close(pipe_fd2[0]);
        close(pipe_fd2[1]);
        close(pipe_fd1[0]);

        val = dup2(pipe_fd1[1], STDOUT_FILENO);       // replace stdout with write part of 1st pipe

        if (val == -1) {
            freeArray(com1Arr, wordNum1 + 2);
            freeArray(com2Arr, wordNum2 + 2);
            freeArray(com3Arr,wordNum3+2);
            close(pipe_fd1[1]);
            fprintf(stderr, "dup2 failed1\n");
            exit(1);
        }

        //executing the first command
        if (execvp(com1Arr[0], com1Arr) < 0) {
            perror("command not found \n");
        }

        // close all ends of pipes and stop the first process
        close(pipe_fd1[1]);
        exit(0);

    }//------------------
    else /*parent,
            after child1 executed the commend, create child 2 to execute the second command */
    {
        child2 = fork();
        if (child2 == -1) {
            perror("process creating failed");
            freeArray(com1Arr, wordNum1 + 2);
            freeArray(com2Arr, wordNum2 + 2);
            freeArray(com3Arr, wordNum3 + 2);
            exit(1);
        }
            //CHILD (2)
        else if (child2 == 0)
        {
            close(pipe_fd1[1]);
            close(pipe_fd2[0]);

            val = dup2(pipe_fd1[0], STDIN_FILENO);   // replace stdin with read end of 1st pipe

            if (val == -1) {
                freeArray(com1Arr, wordNum1 + 2);
                freeArray(com2Arr, wordNum2 + 2);
                freeArray(com3Arr, wordNum3 + 2);
                close(pipe_fd1[0]);
                close(pipe_fd2[1]);
                fprintf(stderr, "dup2 failed1\n");
                exit(1);
            }

            val = dup2(pipe_fd2[1], STDOUT_FILENO);              // replace stdout with write end of 2nd pipe


            if (val == -1) {
                freeArray(com1Arr, wordNum1 + 2);
                freeArray(com2Arr, wordNum2 + 2);
                freeArray(com3Arr, wordNum3 + 2);
                close(pipe_fd1[0]);
                close(pipe_fd2[1]);
                fprintf(stderr, "dup2 failed1\n");
                exit(1);
            }

            //executing the second command
            if (execvp(com2Arr[0], com2Arr) < 0) {
                perror("command not found \n");
            }

            // close all ends of pipes and stop the second process
            close(pipe_fd1[0]);
            close(pipe_fd2[1]);
            exit(0);

        }//-------------------------
        else
            /*parent, after child 1 and 2 executed, creates the third process that will execute the third command */
        {
            child3 = fork();
            if (child3 == -1) {
                perror("process creating failed");
                freeArray(com1Arr, wordNum1 + 2);
                freeArray(com2Arr, wordNum2 + 2);
                freeArray(com3Arr, wordNum3 + 2);
                exit(1);
            }
                //CHILD (3)
            else if (child3 == 0)
            {
                close(pipe_fd1[0]);
                close(pipe_fd1[1]);
                close(pipe_fd2[1]);

                val = dup2(pipe_fd2[0], STDIN_FILENO);     // replace stdin with input read of 2nd pipe

                if (val == -1) {
                    freeArray(com1Arr, wordNum1 + 2);
                    freeArray(com2Arr, wordNum2 + 2);
                    freeArray(com3Arr, wordNum3 + 2);
                    close(pipe_fd2[0]);
                    fprintf(stderr, "dup2 failed\n");
                    exit(1);
                }

                //executing the third command
                if (execvp(com3Arr[0], com3Arr) < 0) {
                    perror("command not found \n");
                }

                // close all ends of pipes and stop the third process

                close(pipe_fd2[0]);
                exit(0);
            } //----------------------------------

        }
    }

    // only the parent gets here and waits for 3 children to finish
    close(pipe_fd1[0]);
    close(pipe_fd1[1]);
    close(pipe_fd2[0]);
    close(pipe_fd2[1]);

    for (int i = 0; i < 3; i++)
        wait(NULL);

    freeArray(com1Arr, wordNum1 + 2);
    freeArray(com2Arr, wordNum2 + 2);
    freeArray(com3Arr,wordNum3+2);

    return;
}
/* ----------------------------------------------------*/



/* HELP METHODS: */

int wherePipe(char* str, int from)
{
    int p=from+1;
    char c;
    while (p!= strlen(str))
    {
        c=str[p];
        if(c== '|')
            return p;
        p++;
    }
    return -1;
}


void getPrompt(char *p)
{
    //getting the current username:
    register struct passwd *username;
    register uid_t uid;
    uid = geteuid ();
    username = getpwuid (uid);
    if (username->pw_name == NULL)
        strcpy(p, "null");
    else
        strcpy(p, (char *)username->pw_name);

    //getting the current dir:
    char path[PATH_MAX];
    if (getcwd(path, sizeof(path)) == NULL)
        strcpy(path, "null");

    //making the prompt's format:
    strcat(p, "@");
    strcat(p, path);
    strcat(p, ">");
    return;
}

void buildArray(char**arr, char*com, int size)
{
    char *curWord;
    int i = 0; //will be index that runs on the sentence

    int j;
    for (j = 0; j < size; ++j) {
        curWord = getWord(com, strlen(com), &i);
        arr[j] = (char*)malloc(strlen(curWord) * sizeof(char));
        if (arr[j] == NULL) {
            puts("There was a problem with the dynamic assigning");
            exit(1);
        }

        stpcpy(arr[j], curWord);
        free(curWord);
    }
    arr[j] = NULL;
    arr[j + 1] = "\0";
    return;
}

int howManyWords(char *strInput, int inputLen)
{
    int wordCount = 0;
    int firsChar = 0, lastChar; //will be the first and last char of each word

    while (firsChar < inputLen) {
        //finding the first char of the current word:
        while (firsChar < inputLen && (' ' == strInput[firsChar]))
            firsChar++;
        if ((firsChar == inputLen-1)||(strInput[firsChar]=='\n'))
            break;
        wordCount++;//counting the words in the sentence

        lastChar = firsChar;
        //finding the last char of the word
        if('"' ==strInput[firsChar]) //to support block of commands in " "
        {
            lastChar++;
            while ((lastChar < inputLen) && ('"' != strInput[lastChar]))
                lastChar++;
        }
        else {
            while ((lastChar < inputLen) && (' ' != strInput[lastChar]))
                lastChar++;
        }

        firsChar = lastChar + 1;
    }

    return wordCount;
}

char *getWord(char *inputString ,int inputLen, int* i)
{
    {
        int firstChar=*i; int lastChar=*i; //will be the first and last char indexes of each word
        char *curWord =NULL;
        //finding the first char of the current word:
        while ((firstChar < inputLen) && (' ' == inputString[firstChar]) )
            firstChar++;

        lastChar = firstChar;
        //finding the last char of the current word
        if('"' ==inputString[firstChar]) //to support block of commands in " "
        {
            lastChar++;
            while ((lastChar < inputLen) && ('"' != inputString[lastChar]))
                lastChar++;
        }
        else {
            while ((lastChar < inputLen) && (' ' != inputString[lastChar]))
                lastChar++;
        }

        *i=lastChar;
        if(lastChar==inputLen) //remove \n
            lastChar--;
        curWord = (char *) malloc(lastChar - firstChar+1);
        if (curWord == NULL) {
            perror("There was a problem with the dynamic assigning");
            exit(1);
        }

        //create a substring from inputString of the current word
        memset(curWord, '\0', sizeof(curWord));
        strncpy(curWord, (inputString + firstChar), lastChar-firstChar);


        return  curWord;
    }
}

void freeArray(char**arr, int size)
{
    for (int i = 0; i < size-1; ++i)
        free(arr[i]);

    free(arr);
}

