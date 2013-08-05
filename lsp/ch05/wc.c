#include <stdio.h>
#include <stdlib.h> // malloc(), free()
#include <unistd.h> // open(), read(), and close()
#include <fcntl.h> // O_RDONLY flag
#include <ctype.h> // isalnum()

/* wc.c
 * a function to count the characters, words and lines in a text file. 
 * this is meant to be a simplified approximation of the 'wc' system utility.
 *
 * usage: 
 *   wc <path>
 *
 * intent: 
 *   open the file named on the command line and read a chunk of data from it.
 *   for each chunk, 
 *     iterate over each character
 *     classify each character and keep count of each type of character.
 *     use a simple state machine to facilitate word counts.
 *     any sequence of non-whitespace characters is counted as a word.
 */




int wc(int argc, char** argv)
{
    int fd = -1;
    int bufsize = 4096;
    int length; // how many bytes read?
    char* buf = 0;
    char* filename;

    // accumulators
    int char_count = 0;
    int word_count = 0;
    int line_count = 0;

    // state
    enum {INITIAL, WORD, SPACE} state;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        goto out;
    }

    filename = argv[1]; // convenience
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "failed to open file\n");
        goto out;
    }

    buf = malloc(bufsize);
    if (0 == buf) {
        fprintf(stderr, "failed to allocate a read buffer.\n");
        goto out;
    }


    state = INITIAL;
    while (0 < (length = read(fd, buf, bufsize))) {
        int i;
        char c;
        
        char_count += length;

        for (i=0; i<length; ++i) {
            c = buf[i]; // convenience.
            
            // count lines
            if ('\n' == c) {
                ++line_count;
            }

            // count words
            switch (state) {

            case INITIAL:
                if (!isspace(c)) {
                    ++word_count;
                    state = WORD;
                } else {
                    state = SPACE;
                }
                break;

            case WORD:
                if (isspace(c)) {
                    state = SPACE;
                }
                break;

            case SPACE:
                if (!isspace(c)) {
                    ++word_count;
                    state = WORD;
                }
                break;

            default:
                // should never get here. 
                fprintf(stderr, "Somebody has disturbed the space-time continuum\n");
                state = INITIAL;
                break;
            } // switch
        } // for each character
    } // while

    // print out the counts
    //printf("characters: %d, words: %d, lines: %d\n", char_count, word_count, line_count);
    // make it look like the output of wc
    printf("  %d %d %d %s\n", line_count, word_count, char_count, filename);

out:
    if (buf) {
        free(buf);
    }

    if (0 < fd) {
        close(fd);
    }

    return 0;
}
