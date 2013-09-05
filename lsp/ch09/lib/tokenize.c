#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// tokenize the string contained in buf.  
// iterate over string and separate it in to tokens
// where tokens are delimited by chars from the set 'delims'
// remove all delimiters and terminate each token with a null char.
// pack the tokens back into the buffer. 
// in essence, this function transforms the contents of buf from a 
// single null teminated string to a packed array of null terminated
// strings. the tken strings follow one right after another. 
// returns the number of tokens in the string.
// assumes that buf contains a null terminated string. 
// bufsize should be at least strlen(buf) + 1
int tokenize(char* buf, size_t bufsize, const char* delims, size_t* token_bufsize)
{
    int token_count; // how many tokens were found
    char* begin; // points to first char in token
    char* end; // points to the first byte after token
    char* dest; // where this token is placed in result
    size_t delim_bytes;
    size_t token_bytes; 
    size_t total_token_bytes; 

    // initialize local vars
    token_count = 0;
    begin = buf;
    end = buf;
    dest = buf;
    token_bytes = 0;
    total_token_bytes = 0;

    // enforce pre-conditions: 
    if (!buf || !delims) {
        goto out;
    }

    delim_bytes = strspn(end, delims);

    while (end - buf < bufsize) {
        begin = end + delim_bytes;
        if (0 == *begin) break;
        token_bytes = strcspn(begin, delims);
        end = begin + token_bytes;
        delim_bytes = strspn(end, delims);

        memmove(dest, begin, token_bytes);
        dest = dest + token_bytes;
        *dest = 0;
        ++dest;

        total_token_bytes += token_bytes;

        if (token_bytes != 0) {
            ++token_count;
        }
    }

    // pad the reamaining space in buffer with zeros.
    size_t pad_bytes = bufsize - (total_token_bytes + token_count);
    if (pad_bytes) {
        memset(dest, 0, pad_bytes);
    }

out:
    if (token_bufsize) {
        *token_bufsize = total_token_bytes + token_count;
        //*token_bufsize = bufsize - pad_bytes; // == total_token_bytes + token_count;
    }
    return token_count;
}



// given a token buffer like that produced by the tokenize() function
// initialize the argv array of pointers with up to argc pointers to
// tokens contained in the token buffer, buf. 
// note: The caller MUST pre-allocate the array, argv such that 
// the array has space for at least argc+1 pointers. This is because 
// convention demands that argv[argc] == 0.
// Caller should also initialize the whole array to zeros. 
int init_argv(char** argv, int argc, const char* buf, size_t bufsize)
{
    int i;
    size_t offset;

    i = 0;
    offset = 0;

    while (i < argc && offset < bufsize && buf[offset]) {
        argv[i] = (char*)(buf + offset);
        offset = offset + strlen(argv[i]) + 1;
        ++i;
    }
    argv[i] = 0;

    return i;
}

