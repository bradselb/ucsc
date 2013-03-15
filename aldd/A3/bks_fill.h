#if !defined BKS_FILL_H
#define BKS_FILL_H

// fill size bytes in destination buffer with fist len
// characters from source string. Repeat source if necessary.
// returns zero on success. less than zero if passed bummer params.
int bks_fill(void* dest, unsigned int size, const char* str, int len);

#endif // !defined BKS_FILL_H
