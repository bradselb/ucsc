#include <stdio.h>
#include <stdlib.h> // malloc(), free()
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h> // open(), read(), and close()
#include <fcntl.h> // O_RDONLY flag
#include <ctype.h> // isspace()

/* word_couunt.c
 * a simple program to count the characters, words and lines in a text file. 
 * this is meant to be a simplified approximation of the 'wc' system utility.
 * additionally, this is a academic example of the use of pthreads. I do not
 * think one would actually choose to use threads in this case but, the 
 * word count problem does lend itself nicely to the present task.
 *
 * usage: 
 *   word_count <path>
 *
 * intent: 
 *   open the file named on the command line and read a chunk of data from it.
 *   for each chunk, 
 *     iterate over each character
 *     classify each character and keep count of each type of character.
 *     use a simple state machine to facilitate word counts.
 *     any sequence of non-whitespace characters is counted as a word.
 */


// -------------------------------------------------------------------------
// globals. these are all magically initialized to zero.
int terminate; // main tells worker that it is quitting time.
int char_count;
int word_count;
int line_count;

// -------------------------------------------------------------------------
// yield the cpu for a liitle bit.
void yield()
{
    static struct timespec t;
    t.tv_sec = 0;
    t.tv_nsec = 1000000; // 1 milliseconds
    nanosleep(&t, 0);
}


// -------------------------------------------------------------------------
struct ring_buffer
{
    int get; // where to get the next char
    int put; // where to put the next char
    int size; // how many chars can the buffer hold?
    pthread_mutex_t lock; // synchronize access to the internals.
    char buf[0];
};

// -------------------------------------------------------------------------
struct ring_buffer* create_ring_buffer(int bufsize)
{
    struct ring_buffer* rb;
    size_t size;
    void* p;
    int ec;

    rb = 0;

    if (bufsize < 1) {
        goto out;
    }

    size = sizeof (struct ring_buffer) + bufsize;
    p = malloc(size);
    if (!p) {
        goto out;
    }

    memset(p, 0, size);
    rb = (struct ring_buffer*)p;
    rb->size = bufsize;
    ec = pthread_mutex_init(&rb->lock, NULL);
    if (ec != 0) {
        free(p);
        rb = 0;
    }


out:    
    return rb;
}

// -------------------------------------------------------------------------
void destroy_ring_buffer(struct ring_buffer* rb)
{
    if (rb) {
        pthread_mutex_destroy(&rb->lock);
        free(rb);
        rb =0;
    }
}

// -------------------------------------------------------------------------
// if adding one more char to the ring would make get == put then it is full.
// caller must hold the lock! 
int is_full(struct ring_buffer* rb)
{
    return ((rb->put + 1)%rb->size == rb->get);
}

// -------------------------------------------------------------------------
// buffer is empty when the put offset is identical to the get offset.
// caller must hold the lock! 
int is_empty(struct ring_buffer* rb)
{
    return (rb->get == rb->put);
}

// -------------------------------------------------------------------------
void put(struct ring_buffer* rb, char c)
{
    if (rb && 0 == pthread_mutex_lock(&rb->lock)) {
        while (is_full(rb)) {
            // if the buffer is full, wait a while for the other thread to 
            // pull some out....but, do not sleep holding the lock!
            pthread_mutex_unlock(&rb->lock);
            yield();
            pthread_mutex_lock(&rb->lock);
        }
        // we have the lock and the buffer is not empty.
        *(rb->buf + rb->put) = c;
        rb->put = (rb->put + 1)%rb->size;
        pthread_mutex_unlock(&rb->lock);
    }
}

// -------------------------------------------------------------------------
int get(struct ring_buffer* rb, char* c)
{
    int got_one;

    got_one = 0; // did not get a char yet.
    if (rb && c && 0 == pthread_mutex_lock(&rb->lock) ) {
        if (is_empty(rb)) {
            // we have the lock but the buffer is empty
            pthread_mutex_unlock(&rb->lock);
            yield(); // let the other guy have a chance
        } else {
            // we have the lock and the buffer is not empty.
            *c = *(rb->buf + rb->get);
            rb->get = (rb->get + 1)%rb->size;
            pthread_mutex_unlock(&rb->lock);
            got_one = 1;
        }
    }
    return got_one;
}

// -------------------------------------------------------------------------
// this is the thread function.
void* word_counter(void* arg)
{
    int got_one;
    struct ring_buffer* rb = (struct ring_buffer*)arg;
    enum {SPACE, WORD} state;


    if (!rb) {
        // not much to do here. bail out.
        goto out;
    }

    state = SPACE;

    while (1) {
        char c;
        got_one = get(rb, &c);
        if (got_one) {
            // count chars
            ++char_count;

            // count lines
            if ('\n' == c) {
                ++line_count;
            }

            // count words
            switch (state) {

            case SPACE:
                if (!isspace(c)) {
                    ++word_count;
                    state = WORD;
                }
                break;

            case WORD:
                if (isspace(c)) {
                    state = SPACE;
                }
                break;


            default:
                // should never get here. 
                fprintf(stderr, "Somebody has disturbed the space-time continuum\n");
                state = SPACE; // fudge it and start over.
                break;
            } // switch
         }
         
         // if terminate is true, main thread is no longer putting into buffer
         // so, we can check empty without holding the lock.
         if (terminate && is_empty(rb)) {
            break; 
         }
    } // while


out:
    return arg; // meaningless but, it is something.
}



// -------------------------------------------------------------------------
int main(int argc, char** argv)
{
    char* filename = 0;
    FILE* file = 0;
    struct ring_buffer* buf = 0;
    int bufsize = 1024;
    pthread_t thread;
    int ec;
    int c;

    // we need at least one command line arg...the filename.
    if (argc < 2) {
        fprintf(stderr, "usage: %s <filename> [<bufsize>]\n", argv[0]);
        goto out;
    }

    // pick up optional second cmd line arg, the internal buffer size
    if (2 < argc) {
        long n;
        errno = 0;
        n = strtol(argv[2], 0, 0);
        if (0 == errno) {
            bufsize = n;
        }
    }

    // create the ring buffer
    buf = create_ring_buffer(bufsize);
    if (!buf) {
        fprintf(stderr, "could not allocate memmory for internal buffer\n");
        goto out;
    }

    // try to open the file.
    filename = argv[1]; // convenience
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "failed to open file '%s'\n", filename);
        goto out;
    }

    // kick off a thread to count the words, etc...
    ec = pthread_create(&thread, NULL, word_counter, buf);
    if (0 != ec) {
        fprintf(stderr, "pthread_create() returned: %d\n", ec);
        goto out;
    }


    // read chars from the file and stuff them into the buffer.
    while (EOF != (c = fgetc(file))) {
        put(buf, c);
    } // while

    // tell the thread we're done.
    // well...we need to wait a bit to give him a change to finish...
    yield();
    terminate = 1;

    // wait for the thread to finish up
    ec = pthread_join(thread, NULL);
    if (0 != ec) {
        fprintf(stderr, "pthread_join() returned: %d\n", ec);
        //goto out;
    }

    // print out the counts -- make it look like the output of wc
    printf("  %d %d %d %s\n", line_count, word_count, char_count, filename);

out:
    if (buf) {
        destroy_ring_buffer(buf);
    }

    if (file) {
        fclose(file);
    }

    return 0;
}
