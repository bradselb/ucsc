#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



// Brad Selbrede
// Linux System Programing, Summer 2013
// Assignment #2, question #5
// Write a simple program that prints "Hello World" in the 
// background five times on a 10 second interval.
// this is a gratuitous re-use of q2.c



int main(int argc, char* argv[], char* env[])
{
    pid_t pid;
    pid_t child = -1; // wait for any child by default.
    int status; // child's return status.


    // throw off a child process to do the hello world loop
    pid = fork();
    if (0 == pid) {
        // this is the child process
        int i;
        for (i=0; i<5; ++i) {
            puts("Hello World");
            sleep(10);
        }
    } else if (pid < 0) {
        // an error
        perror("fork"); 
    } else {
        // successfully started a child process
        child = pid; // save this child's pid so that we can wait for it later.
        printf("successfully created child process (pid: %ld)\n", child);
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
