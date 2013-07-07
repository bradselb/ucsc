#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    //show the PATH envirronment variable
    printf("PATH=%s\n", getenv("PATH"));
    return 0;
}
