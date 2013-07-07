#include <stdio.h>
#include <stdlib.h>

// Brad Selbrede
// Linux System Programing, Summer 2013
// Assignment #3, question #1
// Write a simple program that prints "Hello World" in the 
// background five times on a 10 second interval.

int main(int argc, char* argv[])
{
    //show the PATH envirronment variable
    printf("PATH=%s\n", getenv("PATH"));
    return 0;
}
