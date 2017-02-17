// hw05
// Alyxandra Spikerman

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

void execute(char** args);
char** splitLine(char* line);
void chooseOp(char** args);
void opAnd(char** args);
void opOr(char** args);
void opInput(char** args);
void opOutput(char** args);
void opPipe(char** args);
void opBackground(char** args);
void opSemi(char** args);

int main(int argc, char* argv[]) {
    char cmd[256];

    if (argc == 1) { // no file arguments
        printf("nush$ "); // print out prompt for user
        fflush(stdout);
        while (fgets(cmd, 256, stdin) != NULL) {
            // if user inputs "exit", they exit the shell
            if ((strncmp(cmd, "exit", 4)) == 0) {
                break;
            }
            char** commands = splitLine(cmd); // split the line by " "
            chooseOp(commands); // choose which operation given the commands
            printf("nush$ ");   // print prompt for user
            fflush(stdout);
            free(commands);
        }
    }
    else { // file arguemnts
        FILE* f = fopen(argv[1], "r"); // open file
        while (fgets(cmd, 256, f) != NULL) {
            char** commands = splitLine(cmd); // split the line by " "
            chooseOp(commands); // choose which operation given the commands
            free(commands);
        }
        fclose(f); // close file
    }
    return 0;
}

/*
    execute: creates a fork process and runs the command
    @param args: list of command and its arguments to run
*/
void execute(char** args) {
    int cpid;

    if ((cpid = fork())) {
        // parent process
        // Child may still be running until we wait.
        int status;
        waitpid(cpid, &status, 0);
    }
    else {
        // child process
        if (args[0] != NULL) {
            execvp(args[0], args);
        }
    }
}

/*
    splitLine: splits the given line into an array of strings
    @param line: line inputted by user to split
*/
char** splitLine(char* line) {
    // allocate space for the array cmds
    char** cmds = (char **)malloc(10 * (sizeof(char*)));
    char* readLine; // where the strings will be temp stored
    int c = 0;

    readLine = strtok(line, " \n"); // break the line based on " \n" delimiters
    while (readLine != NULL) {
        cmds[c] = readLine;
        c++;
        readLine = strtok(NULL, " \n");
    }
    cmds[c] = NULL;
    return cmds; // return the broken line
}

/*
    chooseOp: choose the operation based on the array of arguments
    @param args: list of command and its arguments to run
*/
void chooseOp(char** args) {
    int i = 0; // counter
    int activated = 0; // do we have a specific command to run?

    while ((args[i] != NULL) && (activated != 1)) {
        if (strncmp(args[i], "cd", 2) == 0) { // cd
            activated = 1;
            chdir(args[1]);
            break;
        }
        else if (strncmp(args[i], "&&", 2) == 0) { // AND
            activated = 1;
            opAnd(args);
            break;
        }
        else if (strncmp(args[i], "||", 2) == 0) { // OR
            opOr(args);
            activated = 1;
            break;
        }
        else if (strncmp(args[i], "&", 1) == 0) { // background
            activated = 1;
            opBackground(args);
            break;
        }
        else if (strncmp(args[i], "<", 1) == 0) { // redirect input
            activated = 1;
            opInput(args);
            break;
        }
        else if (strncmp(args[i], ">", 1) == 0) { // redirect output
            activated = 1;
            opOutput(args);
            break;
        }
        else if (strncmp(args[i], "|", 1) == 0) { // pipe
            activated = 1;
            opPipe(args);
            break;
        }
        else if (strncmp(args[i], ";", 1) == 0) { // pipe
            activated = 1;
            opSemi(args);
            break;
        }
        i++;
    }
    if (activated == 0) { // we didn't have a specific command to run
        execute(args); // run the command inputted
    }
}

/*
    opAnd: deal with the operation &&
    @param args: list of command and its arguments to run
*/
void opAnd(char** args) {
    char* argument1 = args[0]; // first argument to check

    // if calling argument1 returns successfully, then run the next command
    if (system(argument1) == 0) {
        char* argument2[3] = {args[2], args[3]};
        chooseOp(argument2);
    }
}

/*
    opOr: deal with the operation ||
    @param args: list of command and its arguments to run
*/
void opOr(char** args) {
    char* argument1 = args[0]; // first argument to check

    // if calling argument1 returns unsuccessfully, then run the next command
    if (system(argument1) != 0) {
        char* argument2[3] = {args[2], args[3]};
        chooseOp(argument2);
    }
}

/*
    opInput: redirects the input to the first command
    @param args: list of command and its arguments to run
*/
void opInput(char** args) {
    char* cmds[5];
    int i = 0;

    while ((args[i] != NULL) && (strncmp(args[i], "<", 1) != 0)) {
        cmds[i] = args[i];
        i++;
    }
    cmds[i] = NULL; // null terminate the cmds
    if (args[i+1] != NULL) {
        int f = open(args[i+1], O_RDWR); // open input file
        int cpid;

        if ((cpid = fork())) {
            // parent process
            // Child may still be running until we wait.
            int status;
            waitpid(cpid, &status, 0);
        }
        else {
            // child process
            dup2(f, 0); // redirect the input to the command
            if (cmds[0] != NULL) {
                execvp(cmds[0], cmds);
            }
        }
        close(f); // close the file
    }
}

/*
    opOutput: redirects the output to a file
    @param args: list of command and its arguments to run
*/
void opOutput(char** args) {
    char* cmds[5];
    int i = 0;

    while ((args[i] != NULL) && (strncmp(args[i], ">", 1) != 0)) {
        cmds[i] = args[i];
        i++;
    }
    cmds[i] = NULL; // null terminate the cmds

    if (args[i+1] != NULL) {
        int f = open(args[i+1], O_RDWR | O_CREAT, S_IRWXU); // open output file
        int cpid;

        if ((cpid = fork())) {
            // parent process
            // Child may still be running until we wait.
            int status;
            waitpid(cpid, &status, 0);
        }
        else {
            // child process
            dup2(f, 1); // redirect the output of the command
            if (cmds[0] != NULL) {
                execvp(cmds[0], cmds);
            }
        }
        close(f); // close the file
    }
}

/*
    opPipe: redirects the output of a command to the input of another command
    @param args: list of command and its arguments to run
*/
void opPipe(char** args) {
    char* cmds1[7];
    char* cmds2[7];
    int i = 0, j = 0;

    while ((args[i] != NULL) && (strncmp(args[i], "|", 1) != 0)) {
        cmds1[i] = args[i];
        i++;
    }
    cmds1[i] = NULL; // null terminate
    i++;
    while (args[i] != NULL) {
        cmds2[j] = args[i];
        i++;
        j++;
    }
    cmds2[j] = NULL; // null terinate

    int fd[2];
    pipe(fd);
    int cpid;
    int ppid;

    if ((cpid = fork()) == 0) {
        int pr = fd[0];
        int pw = fd[1];

        if ((ppid = fork()) == 0) {
            // child process
            close(pw);
            dup2(pr, 0);
            if (cmds2[0] != NULL) {
                execvp(cmds2[0], cmds2); // all other cmds use execvp()
            }
        }
        else {
            // parent process
            close(pr);
            dup2(pw, 1);
            if (cmds1[0] != NULL) {
                execvp(cmds1[0], cmds1); // all other cmds use execvp()
            }
        }
    }
    else {
        int status;
        waitpid(cpid, &status, 0);
    }
}

/*
    opBackground: puts the process in the background
    @param args: list of command and its arguments to run
*/
void opBackground(char** args) {
    char* cmds[5];
    int i = 0;

    while ((args[i] != NULL) && (strncmp(args[i], "&", 1) != 0)) {
        cmds[i] = args[i];
        i++;
    }
    cmds[i] = NULL; // null terminate the cmds

    int cpid;

    if ((cpid = fork())) {
        // parent process
        // do not wait for child
        setpgid(0, 0);
    }
    else {
        // child process
        if (cmds[0] != NULL) {
            execvp(cmds[0], cmds); // all other cmds use execvp()
        }
    }
}

/*
    opSemi: executes both commands separated by a semicolon
    @param args: list of command and its arguments to run
*/
void opSemi(char** args) {
    char* cmds1[7];
    char* cmds2[7];
    int i = 0, j = 0;

    while ((args[i] != NULL) && (strncmp(args[i], ";", 1) != 0)) {
        cmds1[i] = args[i];
        i++;
    }
    cmds1[i] = NULL; // null terminate
    i++;
    while (args[i] != NULL) {
        cmds2[j] = args[i];
        i++;
        j++;
    }
    cmds2[j] = NULL; // null terinate

    execute(cmds1);
    execute(cmds2);
}
