#include <stdio.h> // fputs()

#include "slist.h"


int main(int argc, char** argv)
{
	int rc = 0;
	struct slist* head = 0;

	head = slist_alloc();
	if (!head) {
		fprintf(stderr, "failed to allocate stringlist\n");
		goto end;
	}

	slist_push_front(head, "first");
	slist_push_front(head, "second");
	slist_push_front(head, "third");

	slist_foreach(head, puts);

	slist_free(head);


end: 
	return 0;
}
