#ifndef __UTILS_H__
#define __UTILS_H__

char **tokenize(char *str, char *delims);
void print_err(char *fmt, ...);
void *emalloc(size_t size);
int asprintf(char **str, char *fmt, ...);
void chomp(char *str);
unsigned int strv_length(char **strv);
void strv_free(char **strv);

#define die(fmt, ...)	{ print_err(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }

#endif /* __UTILS_H__ */
