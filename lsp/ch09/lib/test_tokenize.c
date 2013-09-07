#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #include <argz.h>

#include "tokenize.h"


int main(int argc, char* argv[])
{
    const char* delims = " ,;:=\n\t.";
    char s1[] = "this is a simple case with eight tokens";
    char s2[] = ";, a more =difficult,case:=with seven,=;    tokens.\n";
    
    int token_count = 0;
    size_t token_bufsize = 0;
    char* tokv[10];
    int tokc = sizeof tokv / sizeof tokv[0];
    int count;

    memset(tokv, 0, sizeof tokv);

    token_count = tokenize(s1, sizeof s1, delims, &token_bufsize);
    count = init_argv(tokv, tokc, s1, token_bufsize);
    for (int i=0; i<token_count; ++i) {
        printf();
    }



    return 0;
}
