#if !defined(RBF_HELP_H)
#define RBF_HELP_H

// a couple of helper functions used in the implementation of rbfopen(). 
// these are not part of the public interface of rbf but can be used 
// by clients or hidden from them. 

// returns non-zero if the named file exists otherwise, returns zero.
int fexists(const char* path);

// find the size of file. return zero if successful, non-zero otherwise.
int fsize(const char* path, size_t* size);

#endif // !defined(RBF_HELP_H)

