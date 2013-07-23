#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
//#include <sys/wait.h>
#include <pthread.h>



// Brad Selbrede
// Linux System Programing, Summer 2013
// Assignment #4
// Write a simple program that prints "Hello World" in the 
// background five times on a 10 second interval.
// this is a gratuitous re-use of q5.c from assignment #2
// this is meant to be a very simplistic demonstation of the
// use of the pthreads library.


// forward declarations;
struct say_something_args;
void* say_something(void* arg);
struct say_something_args* create_say_args(const char* what, int how_many, int interval, int nr);
void free_say_args(struct say_something_args*);

const int nr_threads_to_start = 2;
const char* message = "Hello, World!";

int main(int argc, char* argv[], char* env[])
{
    //int rc;
    int nr_threads_started;
    int i;
    pthread_t thread[nr_threads_to_start];
    struct say_something_args* arg[nr_threads_to_start + 1];
    void* not_interested;

    // create some args to send to the threads. 
    for (i=0; i<nr_threads_to_start; ++i) {
	arg[i] = create_say_args(message, 5, 10, i);
    }
    arg[nr_threads_to_start] = 0; //mark the end of the args list.


    // throw off a couple of threads to say hello world a few times each.
    i = 0;
    nr_threads_started = 0;
    while (arg[i]) {
	int ec;
	ec = pthread_create(&thread[i], 0, say_something, arg[i]);
    	if (ec) {
	    errno = ec;
	    perror("pthread_create");
	    break; // don't try to start any more threads.
	}
	++nr_threads_started;
	++i;
    }

    // now wait for the threads to terminate.
    for (i=0; i<nr_threads_started; ++i) {
	pthread_join(thread[i], &not_interested);
    }

    return 0;
}




// define the args structure
struct say_something_args {
    const char* what;
    int how_many_times;
    int interval;
    int nr;
};

// define the thread funtion
void* say_something(void* arg)
{
    if (arg) {
	struct say_something_args* params;
	int i;
	params = (struct say_something_args*)arg;
	for (i=0; i<params->how_many_times; ++i) {
	    printf("Thread number %d says, '%s'\n", params->nr, params->what);
	    sleep(params->interval);
	}
    }
    return arg;
}

// constructor for args struct.
struct say_something_args* create_say_args(const char* what, int how_many, int t, int nr)
{
    struct say_something_args* arg;
    arg = 0;

    // bail out if any param is not good.
    if (! (what && how_many>0 && t>0) )
	goto bail_out;


    arg = malloc(sizeof(struct say_something_args));
    if (arg) {
	arg->what = what;
	arg->how_many_times = how_many;
	arg->interval = t;
	arg->nr = nr;
    }

bail_out:
    return arg;
}


// destructor for args struct.
void free_say_args(struct say_something_args* arg)
{
    if (arg) free(arg);
}

