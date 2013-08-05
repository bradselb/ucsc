#if !defined(STRINGLIST_H)
#define STRINGLIST_H

// a doubly linked list of null terminated strings. 
struct slist;

// allocate the head node of the list. call push function to add nodes.
struct slist* slist_alloc();
// free the entire list - including all of the strings.
// failure to call  this function results in memory leaks!
void slist_free(struct slist* head);

// add a string to the front or back of the list.
int slist_push_front(struct slist* head, const char* s);
int slist_push_back(struct slist* head, const char* s);

// pop a node off the list.
int slist_pop_front(struct slist* head);
int slist_pop_back(struct slist* head);

// get the string contained in the node
const char* slist_string(struct slist* node);

// iterate over the the list.
struct slist* slist_next(struct slist* node);

// call the function for each element in the list, passing the 
// string contained at each node as the argument to the function.
int slist_foreach(struct slist* head, int (*fctn)(const char*));


#endif //!defined(STRINGLIST_H)
