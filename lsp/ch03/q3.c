#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>



// Brad Selbrede
// Linux System Programing, Summer 2013
// Assignment #3, question #3
// 3. Change the above program to call "/bin/ps -f". 
//    Q: Did the pid show up in your output?
//    A: Yes! 
//
// this is a gratuitous re-use of q2.c



int main(int argc, char* argv[], char* env[])
{
    pid_t pid;
    pid_t child = -1; // wait for any child by default.
    int status; // child's return status.



    printf("parent's name: %s, pid: %d\n", argv[0], getpid());

    // throw off a process to call 'ps -f'
    pid = fork();
    if (0 == pid) {
        // this is the child process
        char* args[] = {"ps", "-f",  0};
        int rc;
        rc = execve("/bin/ps", args, env);
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
