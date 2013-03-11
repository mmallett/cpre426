#include <stdio.h>
#include <stdlib.h>

#define K 3

typedef struct node
{
	int data[K];
	struct node * next;
} node_t;

typedef struct queue
{
	int size;
	node_t * head;
	node_t * tail;
} queue_t;

queue_t * init_queue();

void enqueue(queue_t * q, int * data);

int is_empty(queue_t * q);

int * pop(queue_t *);

queue_t * init_queue()
{
	queue_t * ret = (queue_t *) malloc(sizeof(queue_t));
	ret -> size = 0;
	ret -> head = NULL;
	ret -> tail = NULL;
	return ret;
}

void enqueue(queue_t * q, int * data)
{
	node_t * new_node = (node_t *) malloc(sizeof(node_t));
	//new_node -> data = data;
	memcpy(new_node -> data, data, K);
	new_node -> next = NULL;
	if(is_empty(q))
	{
		q -> head = new_node;
		q -> tail = new_node;
	}
	else
	{
		q -> tail -> next = new_node;
		q -> tail = new_node;
	}
	(q -> size) ++;
}

int is_empty(queue_t * q)
{
	return q -> size <= 0;
}

int * pop(queue_t * q)
{
	if( q -> size > 0)
	{
		node_t * popped = q -> head;
		int * ret =  (int*) malloc(K * sizeof(int));
		memcpy(ret, popped -> data, K);
		q -> head = popped -> next;
		free(popped);
		(q -> size) --;
		return ret;
	}
	else
	{
		return 0;
	}
}

int main()
{
	int a[] = {1,2,3};
	int b[] = {9000,9001,9002};
	int c[] = {1000,2000,1500};
	
	queue_t * qt = init_queue();
	
	enqueue(qt, a);
	
	int * ptr;
	
	ptr = pop(qt);
	
	printf("%d %d %d\n", ptr[0], ptr[1], ptr[2]);
	
	enqueue(qt, b);
	
	enqueue(qt, c);
	
	ptr = pop(qt);
	
	printf("%d %d %d\n", ptr[0], ptr[1], ptr[2]);
	
	ptr = pop(qt);
	
	printf("%d %d %d\n", ptr[0], ptr[1], ptr[2]);
	
	return 0;
}
