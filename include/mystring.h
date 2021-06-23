#ifndef MYSTRING_H
#define MYSTRING

int strcmp(const char* a, const char* b);
int strncmp(char* a, char* b, unsigned long n);
void memcpy(char *dest, const char *src, unsigned long n);
void memset(char* a, unsigned int value, unsigned int size);
char *itoa(int value, char *s, int base);
long atoi(char *s);
long strlen(const char *s);
unsigned int vsprintf(char *dest, char *fmt, __builtin_va_list args);

#endif
