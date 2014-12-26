#ifndef __UTILS_H__
#define __UTILS_H__

char **tokenize(char *str, char *delims);
void print_err(char *fmt, ...);
void *emalloc(size_t size);
int asprintf(char **str, char *fmt, ...);
void chomp(char *str);
int strcmp_s(const char *s1, const char *s2);
char *strconcat(const char *s1, ...);
unsigned int strlenv(char **strv);
void strfreev(char **strv);
char *strjoinv(char **strv, const char *separator);

#define die(fmt, ...)	{ print_err(fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }

#endif /* __UTILS_H__ */
