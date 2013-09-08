#if !defined TOKENIZE_H
#define TOKENIZE_H

// both function return the number of tokens in their results
int tokenize(char* buf, size_t bufsize, const char* delims, size_t* token_bufsize);
int init_argv_from_tokenbuf(char** argv, const char* tokbuf, size_t tokbufsize, int tokcount);
int count_tokens(const char* tokbuf, size_t tokbufsize);

void show_tokenbuf(const char* tokbuf, size_t tokbufsize);
#endif // !defined TOKENIZE_H
