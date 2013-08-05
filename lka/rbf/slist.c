#include "slist.h"
#include <stdlib.h> // malloc(), free()
#include <string.h> // strcpy()
#include <stdio.h> // //printf()

// -------------------------------------------------------------------------
struct slist
{
	char* s;
	struct slist* next;
	struct slist* prev;
};



// -------------------------------------------------------------------------
struct slist* slist_alloc()
{
	struct slist* head = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);
	
	head = malloc(sizeof(struct slist));
	if (head) {
		head->s = 0;
		head->next = head;
		head->prev = head;
	}
	return head;
}

// -------------------------------------------------------------------------
void slist_free(struct slist* head)
{
	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);
	if (head) {
		while (head != head->prev) {
			slist_pop_back(head);
		}
		free(head);
	}
}

// -------------------------------------------------------------------------
int slist_push_front(struct slist* head, const char* s)
{
	int rc = 0;
	char* newstr = 0;
	struct slist* node = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);

	if (!(head && s)) {
		rc = -1;
	} else if (!(newstr = malloc(1 + strlen(s)))) {
		rc = -2;
	} else if (!(node = malloc(sizeof(struct slist)))) {
		free(newstr);
		rc = -3;
	} else {
		strcpy(newstr, s);
		node->s = newstr;
		node->prev = head;
		node->next = head->next;
		head->next->prev = node;
		head->next = node;
		rc = 0;
	}
	return rc;
}


// -------------------------------------------------------------------------
int slist_push_back(struct slist* head, const char* s)
{
	int rc = 0;
	char* newstr = 0;
	struct slist* node = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);

	if (!(head && s)) {
		rc = -1;
	} else if (!(newstr = malloc(1 + strlen(s)))) {
		rc = -2;
	} else if (!(node = malloc(sizeof(struct slist)))) {
		free(newstr);
		rc = -3;
	} else {
		strcpy(newstr, s);
		node->s = newstr;
		node->next = head;
		node->prev = head->prev;
		head->prev->next = node;
		head->prev = node;
		rc = 0;
	}
	return rc;
}

// -------------------------------------------------------------------------
int slist_pop_front(struct slist* head)
{
	int rc = 0;
	struct slist* firstnode = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);
	if (!head) {
		rc = -1;
	} else if (head->next != head) {
		firstnode = head->next;
		head->next = firstnode->next;
		firstnode->next->prev = head;
		if (firstnode->s) free(firstnode->s);
		free(firstnode);
	}

	return rc;
}


// -------------------------------------------------------------------------
int slist_pop_back(struct slist* head)
{
	int rc = 0;
	struct slist* endnode = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);
	if (!head) {
		rc = -1;
	} else if (head->prev != head) {
		endnode = head->prev;
		head->prev = endnode->prev;
		endnode->prev->next = head;
		if (endnode->s) free(endnode->s);
		free(endnode);
	}

	return rc;
}



// -------------------------------------------------------------------------
int slist_foreach(struct slist* head, int (*fctn)(const char*))
{
	int rc = 0;
	struct slist* node = 0;

	//printf("(%s:%d) %s()\n", __FILE__, __LINE__, __FUNCTION__);

	if (!head) {
		rc = -1;
	} else if (!fctn) {
		rc = -2;
	} else {
		node = head->next;
		while (node != head) {
			fctn(node->s);
			node = node->next;
		}
	}
	return rc;
}

// -------------------------------------------------------------------------
const char* slist_string(struct slist* node)
{
	const char* s = 0;
	if (node) s = node->s;
	return s;
}

// -------------------------------------------------------------------------
struct slist* slist_next(struct slist* node)
{
	struct slist* next = 0;
	if (node) next = node->next;
	return next;
}

struct slist* slist_prev(struct slist* node)
{
	struct slist* prev = 0;
	if (node) prev = node->prev;
	return prev;
}

