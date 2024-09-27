#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include <stddef.h>

#define COMMAND 1
#define REDIRECT_RIGHT 2
#define REDIRECT_LEFT 3
#define PIPE 4

#define MAXARG 32

void getType (char *line, int *type, int *pos) {
        // loop for how long line is
        for (int i = 0; i < strlen(line); i++){
                // command if non of the others
                *type = COMMAND;
                *pos = 0;

                if (line[i] == '>') {
                        *type = REDIRECT_RIGHT;
                        *pos = i;
                        break;
                }

                if (line[i] == '<') {
                        *type = REDIRECT_LEFT;
                        *pos = i;
                        break;
                }

                if (line[i] == '|') {
                        *type = PIPE;
                        *pos = i;
                        break;
                }
                
        }
}

void parseCommand (char **args, char *line, char delimeter) {
        // PARSE COMMAND

        // marks start of word
        int startWord = 1;
        // pointers to traverse args
        char **a, *l;

        // point a to first arg
        a = &args[0];
        l = line;
        // loop through the input line until we find '\0'
        // every sapce we find is replaced by '\0'
        while (*l != '\0') {
                if (*l != delimeter) {
                        // new word, save pointer for it to args
                        if (startWord) {
                                *a = l;
                                startWord = 0;
                        }
                }

                else {
                        // met space, line in word has ended
                        if (!startWord) {
                                *l = '\0';
                                a++;
                                startWord = 1;
                        }
                }
                // index
                l++;
        }

}

void runCommand (char *line, int type, int pos) {
        // define
        char *args[MAXARG];

        char left[512];
        char right[512];

        int p[2];

        switch (type)
        {
                case COMMAND:
                        // parse command by space
                        parseCommand(&*args, line, ' ');
                        // execute
                        exec(args[0], args);

                        break;
        
                case PIPE:
                        // left of pos of pipe
                        for (int i = 0; i < pos - 1; i++) {
                                left[i] = line[i];
                                left[pos] = '\0';
                        }

                        // right of pos of pipe
                        for (int i = pos-2; i <= strlen(line); i++) {
                                right[i - pos-2] = line[i];
                        }

                        if(pipe(p) < 0) {
                                printf("Pipe Faliled");
                        }

                        // fork off and run each pipe
                        if(fork() == 0) {
                                close(1);
                                dup(p[1]);
                                close(p[0]);
                                close(p[1]);
                                runCommand(left, 1, 0);
                        }
                        if(fork() == 0) {
                                close(0);
                                dup(p[0]);
                                close(p[0]);
                                close(p[1]);
                                runCommand(right, 1, 0);
                        }

                        close(p[0]);
                        close(p[1]);

                        wait(0);
                        wait(0);
                        break;

                case REDIRECT_RIGHT:
                        // for command (left of pos)
                        for (int i = 0; i < pos - 1; i++) {
                                left[i] = line[i];
                                left[pos] = '\0';
                        }

                        // for filename (right of pos)
                        for (int i = pos-2; i <= strlen(line); i++) {
                                right[i - pos-2] = line[i];
                        }

                        // close fd
                        close(1);
                        // open file
                        if (open(right, O_WRONLY|O_CREATE|O_TRUNC) < 0) {
                                printf("Open failed");
                                exit(1);
                        }

                        // execute command
                        runCommand(left, 1, 0);
                        break;

                case REDIRECT_LEFT:
                        
                        // for command (left of pos)
                        for (int i = 0; i < pos - 1; i++) {
                                left[i] = line[i];
                                left[pos] = '\0';
                        }

                        // for filename (right of pos)
                        for (int i = pos-2; i <= strlen(line); i++) {
                                right[i - pos-2] = line[i];
                        }
                        
                        // close fd
                        close(0);
                        // open file
                        if (open(right, O_RDONLY) < 0) {
                                printf("Open failed");
                                exit(1);
                        }

                        // execute command
                        runCommand(left, 1, 0);

                        break;

                default:
                        printf("Type unknown");
                        break;
        }

        
}

int main (void) {
        // define
        char buf[512];

        int type;
        int pos;

        while (1) {
                printf(">>> ");
                
                // get input
                if (gets(buf, sizeof(buf)) != NULL) {
                        // if input not null then remove the \n at the end of buf
                        size_t len = strlen(buf);
                        if (len > 0 && buf[len-1] == '\n') {
                                // add \0 at end of buf
                                buf[--len] = '\0';
                        }
                }

                // if buf is cd
                if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
                    // call chdir, if error, show message
                    if(chdir(buf+3) < 0) {
                        printf("cannot cd %s\n", buf+3);
                    }
                    // go back to top of while loop
                    continue;
                }

                // if buf is exit() exit shell
                if (strcmp("exit()", buf) == 0) {
                        printf("Exiting shell ...");
                        exit(0);
                }

                // run command
                else {
                        // executing command
                        // create a child process
                        int f = fork();

                        if (f < 0) {
                                printf("Fork failed\n");
                                exit(1);
                        }

                        else if (f == 0) {
                                // get type then run
                                getType(buf, &type, &pos);
                                runCommand(buf, type, pos);
                        }

                        else {
                                wait(0);
                        }

                }
                
        }
        exit(0);
}