#if !defined TOKENIZE_H
#define TOKENIZE_H

int tokenize(char* buf, size_t bufsize, const char* delims, size_t* token_bufsize);

int init_argv(char** argv, int argc, const char* buf, size_t bufsize);
#endif // !defined TOKENIZE_H
