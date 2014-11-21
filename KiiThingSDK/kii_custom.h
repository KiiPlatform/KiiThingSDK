#include "kii_cloud.h"

void* kii_malloc(size_t size);
void* kii_memset(void* buf, int ch, size_t n);
void* kii_memcpy(void* buf1, const void* buf2, size_t n);
void kii_free(void* ptr);
kii_char_t* kii_strdup(const kii_char_t* s);
kii_char_t* kii_strncat(kii_char_t* s1, const kii_char_t* s2, size_t n);
size_t kii_strlen(const kii_char_t* str);
kii_char_t* kii_strncpy(kii_char_t *s1, const kii_char_t *s2, size_t n);
void* kii_realloc(void* ptr, size_t size);
int kii_strncmp(const kii_char_t *s1, const kii_char_t *s2, size_t n);
int kii_tolower(int c);
