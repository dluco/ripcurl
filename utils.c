#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "utils.h"

struct queue_node {
	char *data;
	struct queue_node *next;
};

struct queue {
	struct queue_node *head;
	struct queue_node *tail;
};

/*
 * initialize queue
 *
 * Return: none
 */
static void queue_init(struct queue *q)
{
	q->head = q->tail = NULL;
}

/*
 * test if queue is empty (zero elements)
 *
 * Return: non-zero if empty, 0 if non-empty
 */
static int queue_isempty(struct queue *q)
{
	return (q->head == NULL && q->tail == NULL);
}

/*
 * get the length of queue
 *
 * Return: length of queue
 */
static int queue_length(struct queue *q)
{
	struct queue_node *temp;
	int len;

	for (temp = q->head, len = 0; temp; temp = temp->next, len++);

	return len;
}

/*
 * add data to rear of queue
 *
 * Return: 0 on success, -1 on error
 */
static int queue_enqueue(struct queue *q, char *str)
{
	struct queue_node *temp;

	/* create new element */
	temp = emalloc(sizeof *temp);
	if (!temp) {
		return -1;
	}
	temp->data = strdup(str);
	temp->next = NULL;

	if (queue_isempty(q)) {
		/* first element in queue */
		q->head = q->tail = temp;
	} else {
		/* add to rear */
		q->tail->next = temp;
		q->tail = temp;
	}

	return 0;
}

/*
 * remove data from front of queue
 *
 * Return: dynamically allocated string on success, NULL on error
 */
static char *queue_dequeue(struct queue *q)
{
	struct queue_node *temp;
	char *str;

	if (queue_isempty(q)) {
		return NULL;
	}

	/* advance head to second element */
	temp = q->head;	
	q->head = temp->next;

	/* get data */
	str = temp->data;

	free(temp);

	/* update tail if last element was removed */
	if (q->head == NULL) {
		q->tail = NULL;
	}

	return str;
}

/*
 * free a queue's data and delete all elements
 *
 * Return: none
 * */
static void queue_free(struct queue *q)
{
	struct queue_node *temp, *old;

	temp = q->head;

	while (temp) {
		free(temp->data);
		old = temp;
		temp = temp->next;
		free(old);
	}
}

/*
 * tokenize string on delimiters
 *
 * Return: null-terminated array of strings on success, NULL on error
 */
char **tokenize(char *str, char *delims)
{
	char *token, *s, *p, *saveptr;
	char **result;
	int i;
	struct queue q;

	/* make copy of input string */
	s = strdup(str);
	if (!s) {
		/* error */
		return NULL;
	}

	/* initialize queue */
	queue_init(&q);

	for (p = s; (token = strtok_r(p, delims, &saveptr)); p = NULL) {
		/* add to queue */
		queue_enqueue(&q, token);
	}

	free(s);

	/* length of queue is the number of tokens */
	result = emalloc(queue_length(&q) * sizeof *result + 1);
	if (!result) {
		/* error */
		queue_free(&q);
		return NULL;	
	}

	for (i = 0; !queue_isempty(&q); i++) {
		/* empty out queue into result array */
		result[i] = queue_dequeue(&q);
	}
	result[i] = NULL;

	return result;
}

void print_err(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void *emalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (!p) {
		die("malloc of %zu bytes failed\n", size);
	}
	return p;
}

int asprintf(char **str, char *fmt, ...)
{
	va_list argp;
	char one_char[1];
	int len;

	va_start(argp, fmt);
	len = vsnprintf(one_char, 1, fmt, argp);
	if (len < 1) {
		*str = NULL;
		return -1;
	}
	va_end(argp);

	*str = emalloc(len+1);
	if (!str) {
		return -1;
	}

	va_start(argp, fmt);
	vsnprintf(*str, len+1, fmt, argp);
	va_end(argp);

	return len;
}

void chomp(char *str)
{
	char *p;

	if (!str) {
		return;
	}

	for (p = &str[strlen(str)-1]; (*p == '\n' || *p == '\r'); p--) {
		*p = '\0';
	}
}

int strcmp_s(const char *s1, const char *s2)
{
	if (!s1)
		return -(s1 != s2);
	if (!s2)
		return (s1 != s2);
	return strcmp(s1, s2);
}

unsigned int strlenv(char **strv)
{
	unsigned int n;

	if (!strv) {
		return 0;
	}

	for (n = 0; strv[n]; n++);

	return n;
}

void strfreev(char **strv)
{
	int i;

	if (!strv) {
		return;
	}

	for (i = 0; strv[i]; i++) {
		free(strv[i]);
	}

	free(strv);
}

char *strjoinv(char **strv, const char *separator)
{
	char *str, *p;
	int i;
	size_t len, separator_len;

	if (!strv) {
		return NULL;
	}

	if (!separator) {
		separator = "";
	}

	if (*strv) {
		separator_len = strlen(separator);

		/* get length of joined strv */
		for (i = 0, len = 0; strv[i]; i++) {
			len += strlen(strv[i]);
		}
		len += separator_len * (i - 1);

		/* build string */
		str = emalloc(len * sizeof *str + 1);
		if (!str) {
			return NULL;
		}
		p = stpcpy(str, *strv);
		for (i = 1; strv[i]; i++) {
			p = stpcpy(p, separator);
			p = stpcpy(p, strv[i]);
		}
	} else {
		str = strdup("");
	}

	return str;
}
