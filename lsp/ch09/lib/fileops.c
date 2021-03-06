#include <stdio.h>
#include <stdlib.h> // malloc(), free()
#include <string.h>
#include <sys/types.h>
#include <unistd.h> // open(), read(), and close()
#include <fcntl.h> // O_RDONLY flag
#include <errno.h>

#include "mysh_common.h" // get_response_from_server()
//#include "tokenize.h" // show_tokenbuf()  - for debugging only


int decode_filedata(const char* src, void* dest, int bufsize);

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
    unsigned char* rdbuf = 0;
    unsigned int rdbufsize = 512;
    ssize_t rdcount; // how many bytes were read from file.
    char* wrbuf = 0;
    unsigned int wrbufsize = 4096;
    ssize_t wrcount;

    size_t length; // how many bytes to write to destfd
    size_t offset; // where we are in the file


    //fprintf(stderr, "(%s:%d) %s(), local: %s, remote: %s, fd: %d\n", __FILE__, __LINE__, __FUNCTION__, localfilename, remotefilename, destfd);

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
            fprintf(stderr, "(%s:%d) %s(), write() returned: %ld, %s\n", __FILE__, __LINE__, __FUNCTION__, wrcount, strerror(wrcount));
            rc = -1;
            break; // ????
        }

        offset += rdcount;

        get_response_from_server(destfd);
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
    const char* filename;
    size_t offset;
    const char* encoded_data_buf = 0;
    char* decoded_data_buf = 0;
    int decoded_data_bufsize = 1024;
    int decoded_byte_count;
    int fd = -1;
    


    if (argc < 3) {
        rc = -1;
        goto out;
    }

    filename = argv[1];

    errno = 0;
    offset = strtol(argv[2], 0, 0);
    if (errno) {
        rc = -1;
        goto out;
    }

    encoded_data_buf = argv[3];

    decoded_data_buf = malloc(decoded_data_bufsize);
    if (!decoded_data_buf) {
        rc = -1;
        goto out;
    }
    memset(decoded_data_buf, 0, decoded_data_bufsize);

    decoded_byte_count = decode_filedata(encoded_data_buf, decoded_data_buf, decoded_data_bufsize);
    if (decoded_byte_count < 0) {
        rc = -1;
        goto out;
    }

    fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0) {
        rc = -1;
        goto out;
    }

    lseek(fd, offset, SEEK_SET);
    write(fd, decoded_data_buf, decoded_byte_count);
    rc = 0;


out:
    if (fd > 0) {
        close(fd);
    }

    if (decoded_data_buf) {
        memset(decoded_data_buf, 0, decoded_data_bufsize);
        free(decoded_data_buf);
    }

    return rc;
}


// --------------------------------------------------------------------------
int decode_filedata(const char* src, void* dest, int bufsize)
{
    unsigned int v;
    unsigned char* p = (unsigned char*)dest;
    const char* q = src;
    int count = 0;
    char buf[4];

    memset(buf, 0, sizeof buf);

    if (!src || !dest) {
        count = -1;
        goto out;
    }

    while (count < bufsize && *q && *(q+1)) {
        v = 0;

        buf[0] = *q++ ; 
        buf[1] = *q++ ;
        buf[2] = 0;
        buf[3] = 0;

        
        
        v = strtol(buf, 0, 16);

        *p++ = (v & 0x0ff);

        ++count;
    }

out:
    return count;
}

