#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



// Brad Selbrede
// Linux System Programing, Summer 2013
// Assignment #3, question #2
// 2. Write a simple program to print the program's name and pid.
//   2.1. Call "/bin/ls"....I guess that this means, to fork and exec /bin/ls
//   2.2. Call "ls –l /bin/ls" .. .again, I assume that this means to fork and exec


int main(int argc, char* argv[], char* env[])
{
    pid_t pid;
    pid_t child = -1; // wait for any child by default.
    int status; // child's return status.



    printf("parent's name: %s, pid: %d\n", argv[0], getpid());

    // throw off a process to call 'ls -l'
    pid = fork();
    if (0 == pid) {
        // this is the child process
        char* args[] = {"ls", "-l", "--color",  0};
        int rc;
        rc = execve("/bin/ls", args, env);
    } else if (pid < 0) {
        // an error
        perror("first fork"); 
    } else {
        // successfully started a child process
        child = pid; // save this child's pid so that we can wait for it later.
        printf("successfully created child process (pid: %d)\n", child);
        //sleep(1); // poor man's process synchronization.
    }





    pid = waitpid(child, &status, 0);
    if (pid < 0) {
        // an error.
        perror("waitpid()");
    } else {
        printf("child pid: %d, has terminated\n", pid);
    }



    return 0;
}
