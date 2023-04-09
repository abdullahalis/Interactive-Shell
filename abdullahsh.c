#define  _POSIX_C_SOURCE 200809L
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<assert.h>

#define exit(N) {fflush(stdout); fflush(stderr); _exit(N); }

// remove spaces from end of a string
static void trim(char* str) {
    char * end = str;
    while (*end != '\0') {
        end++;
    }
    while (*(end - 1) == ' ') {
        *(end - 1) = '\0';
        end--;
    }
}

void command(char* c) {
    char * sep = NULL;

    // copies of original file descriptors
    int orig_in=dup(0);
    int orig_out=dup(1);
    int orig_err=dup(2);
    int write_fd  = -1;
    int redirected = 0;

    if((sep=strstr(c,"2>"))) {
        *sep=0;
        char * fileout = sep+2;
        trim(fileout);
        
        redirected = 1;
        // open file for writing
        write_fd = open(fileout, O_WRONLY | O_TRUNC | O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (write_fd < 0) {
            perror("Error when opening write file");
            exit(1);
        }
        // redirecy stderr
        if (dup2(write_fd, 2) < 0) {
            perror("Error when performing dup2");
            exit(1);
        }
    }

    if((sep=strstr(c,">"))) {
        *sep=0;
        char * fileout = sep+1;
        
        // get rid of spaces after fileout
        trim(fileout);
        redirected = 1;

        // open file for writing
        write_fd = open(fileout, O_WRONLY | O_TRUNC | O_CREAT,  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (write_fd < 0) {
            perror("Error when opening write file");
            exit(1);
        }
        if (dup2(write_fd, 1) < 0) {
            perror("Error when performing dup2");
            exit(1);
        }
    }

    if((sep=strstr(c,"<"))) {
        *sep=0;
        char * filein = sep+1;
        trim(filein);
        redirected = 1;

        // open file for writing
        int read_fd = open(filein, O_RDONLY, 0);
        if (read_fd < 0) {
            perror("Error when opening write file");
            exit(1);
        }

        // redirect stdin
        if (dup2(read_fd, 0) < 0) {
            perror("Error when performing dup2");
            exit(1);
        }
    }

    char* program = strtok(c," ");

    // return if no arguments passed
    if (program == NULL) {
        return;
    }
    char *args[16]={program};

    for (int i = 1; i < 16; i++) {
        args[i] = strtok(NULL, " ");
        if (args[i] == NULL) {
            break;
        }
    }

    if (!strcmp("echo",program)) {
        for (int i = 1; i < 16; i++) {
            if (args[i] != NULL) {
                printf("%s\n", args[i]);
            }
        }
        return;
    } 
    else {
        // if path is relative
        if (strchr(program, '/') == NULL) {
            char* envPath = getenv("PATH");
            int counter = 1;
            char* paths[500];

            // if only one relative path is given
            if (strchr(envPath, ':') == NULL) {
                paths[0] = envPath;
            }
            // if multiple paths are given
            else {
                char *tok = strtok(envPath, ":");
                paths[0] = tok;
                tok = strtok(NULL, ":");
                while (tok != NULL) {
                    paths[counter] = tok;
                    tok = strtok(NULL, ":");
                    counter++;
                }
            }
            
            for (int i = 0 ; i < counter ; i++) {
                char * path = paths[i];
                char program_with_path[40];
                // generate the full path
                snprintf(program_with_path, sizeof(path) + sizeof(program), "%s/%s", path, program);
                // Try to execute that path
                execv(program_with_path, args);
            }
        }
        // when path is given or no command was found
        execv(args[0], args);
        fprintf(stderr,"abdullahsh: command not found: %s\n",program);
        fflush(stderr);

    }
    if (redirected) {
        dup2(orig_out, 1);
    }
}

void run(char*);

void pipeline(char* head, char* tail) {
    // redirect output of head to input of tail
    int fds[2];
    pipe(fds);
    
    int readfd = fds[0];
    int writefd = fds[1];

    int pid = fork();
    int ret = -1;

    // child
    if (pid == 0) {
        close(readfd);

        // redirect stdout to write to pipe
        dup2(writefd, 1);
        command(head);

        exit(ret);
    }
    //parent
    else {
        close(writefd);
        int orig_stdin = dup(0);
        // redicrect input of tail to readfd
        dup2(readfd, 0);
        // execute
        run(tail);
        wait(&ret);

        //restore stdin
        dup2(orig_stdin, 0);
    }
}

void sequence(char* head, char* tail) {
    run(head);
    run(tail); 
}

void run(char *line) {
      char *sep = 0;

    // find the separator character if it exists
    if((sep=strstr(line,";"))) {        // multiple commands
        *sep=0;
        sequence(line,sep+1); 
    }
    else if((sep=strstr(line,"|"))) {   // pipelining commands
        *sep=0;
        pipeline(line,sep+1);
    }
    else {
        int process_id = -1;
        process_id = fork();
        int ret = -1;
        // parent waits
        if (process_id) {
            wait(&ret);
        }
        else {
            command(line); 
            exit(ret); 
        }
    }
}

int main(int argc, char** argv) {
    char *line=0;
    size_t size=0;

    printf("abdullahsh> ");
    fflush(stdout);

    while(getline(&line,&size,stdin) > 0) {
        line[strlen(line)-1]='\0'; // remove the newline
        run(line);

        printf("abdullahsh> ");
        fflush(stdout);
   }

    printf("\n");
    fflush(stdout);
    fflush(stderr);
    return 0;
}
