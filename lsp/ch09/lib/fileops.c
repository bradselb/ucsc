#include <stdio.h>
#include <stdlib.h> // malloc(), free()
#include <string.h>
#include <unistd.h> // open(), read(), and close()
#include <fcntl.h> // O_RDONLY flag


// --------------------------------------------------------------------------
// given a local filename and a remote filename, generate a sequence of 
// plain text message of the form...
// writefiledata'\0'<remote filename>\'0'<offset>\'0'<encoded data>\'0'
// and write the messages to the destination fd.
// the encoded data will be a plain text representation of up to 512 bytes
// of data read from the local file. The encoded data will, of course, need
// to be decoded by the receiving end. 
int send_putfile_messages(const char* localfilename, const char* remotefilename, int destfd)
{
    int rc;
    int srcfd = -1; // the local file.
    char* rdbuf = 0;
    unsigned int rdbufsize = 512;
    ssize_t rdcount; // how many bytes were read from file.
    char* wrbuf = 0;
    unsigned int wrbufsize = 4096;
    ssize_t wrcount;

    size_t length; // how many bytes to write to destfd
    size_t offset; // where we are in the file



    if (!localfilename  || !remotefilename) {
        rc = -1;
        goto out;
    }

    rdbuf = malloc(rdbufsize);
    if (!rdbuf) {
        rc = -1;
        goto out;
    }
    memset(rdbuf, 0, rdbufsize);

    wrbuf = malloc(wrbufsize);
    if (!wrbuf) {
        rc = -1;
        goto out;
    }
    memset(wrbuf, 0, wrbufsize);

    srcfd = open(localfilename, O_RDONLY);
    if (srcfd < 0) {
        fprintf(stderr, "(%s:%d) %s(), file not found. '%s'\n", __FILE__, __LINE__, __FUNCTION__, localfilename);
    }

    offset = 0;
    // foreach block of data from the source file,
    while (0 < (rdcount = read(srcfd, rdbuf, rdbufsize))) {
        // build up a tokenized command string representing a writefiledata cmd
        offset += rdcount;
        length = 0;

        length += snprintf(wrbuf+length, wrbufsize-length, "writefiledata");
        wrbuf[length++] = 0;

        length += snprintf(wrbuf+length, wrbufsize-length, "%s", remotefilename);
        wrbuf[length++] = 0;

        length += snprintf(wrbuf+length, wrbufsize-length, "%lu", offset);
        wrbuf[length++] = 0;

        for (int i=0; i<rdcount; ++i) {
            length += snprintf(wrbuf+length, wrbufsize-length, "%02x", rdbuf[i]);
        }
        wrbuf[length++] = 0;

        // send the tokenized command string to the destination fd
        wrcount = write(destfd, wrbuf, length);
        if (wrcount < 0) {
            // an error writing...but then what? 
            break; // ????
        }
    }



out:
    if (srcfd > 0) {
        close(srcfd);
    }

    if (wrbuf) {
        free(wrbuf);
    }

    if (rdbuf) {
        free(rdbuf);
    }

    return rc;
}


// --------------------------------------------------------------------------
// writefiledata
// the idea is that we have received a command of the form:
// writefiledata <filename> <offset> <encoded data>
//
// the idea is that we are to decode the data and write the result to the
// named file at the given offset. The length of the decoded data will be
// 512 bytes or less.
// the fd argument is only for this function to say something back to 
// the entity that requested we do the write.

// as I write this function, I'm imagining that this function is called
// as builtin on the server side as a result of the user (on the client)
// saying "put <local path> <remote filename>". 


int writefiledata(int argc, char** argv, int clientfd)
{
    int rc = 0;

    for (int i=0; i<argc; ++i) {
        fprintf(stderr, "%s", argv[i]);
    }

    return rc;
}

