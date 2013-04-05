
#include <stdio.h>
#include <stdlib.h>

typedef struct node
{
	int data;
	struct node * next;
} node_t;

typedef struct queue
{
	int size;
	node_t * head;
	node_t * tail;
} queue_t;

queue_t * init_queue();

void enqueue(queue_t * q, int data);

int is_empty(queue_t * q);

int pop(queue_t *);

int main()
{
	queue_t * qt = init_queue();
	enqueue(qt, 1);
	enqueue(qt, 2);
	enqueue(qt, 3);
	printf("%d\n", pop(qt));
	enqueue(qt, 9000);
	enqueue(qt, 12345);
	while(!is_empty(qt))
	{
		printf("%d\n", pop(qt));
	}
	return 0;
}

queue_t * init_queue()
{
	queue_t * ret = (queue_t *) malloc(sizeof(queue_t));
	ret -> size = 0;
	ret -> head = NULL;
	ret -> tail = NULL;
}

void enqueue(queue_t * q, int data)
{
	node_t * new_node = (node_t *) malloc(sizeof(node_t));
	new_node -> data = data;
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

int pop(queue_t * q)
{
	if( q -> size > 0)
	{
		node_t * popped = q -> head;
		int ret = popped -> data;
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


