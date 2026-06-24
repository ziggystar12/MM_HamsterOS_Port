#pragma once
#include <stdint.h>

/* Freestanding string/memory utilities — no libc dependency.
 * Used everywhere malloc/free/string.h are not available. */

static inline __attribute__((unused))
void *mm_memset(void *dst, int c, uint32_t n)
{
    uint8_t *p = (uint8_t *)dst;
    while (n--) *p++ = (uint8_t)c;
    return dst;
}

static inline __attribute__((unused))
void *mm_memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t *d       = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dst;
}

static inline __attribute__((unused))
uint32_t mm_strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (uint32_t)(p - s);
}

/* Copies at most n-1 chars then zero-fills to n. */
static inline __attribute__((unused))
void mm_strncpy(char *dst, const char *src, uint32_t n)
{
    uint32_t i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    while (i < n) dst[i++] = '\0';
}

/* Case-insensitive ASCII comparison (for FAT 8.3 names). */
static inline __attribute__((unused))
int mm_namecmp(const char *a, const char *b)
{
    for (;;) {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return (int)ca - (int)cb;
        if (!ca) return 0;
        a++; b++;
    }
}

/* ---- ctype replacements ---- */
static inline __attribute__((unused)) int mm_tolower(int c)
    { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
static inline __attribute__((unused)) int mm_toupper(int c)
    { return (c >= 'a' && c <= 'z') ? c - 32 : c; }
static inline __attribute__((unused)) int mm_isprint(int c)
    { return (c >= 0x20 && c < 0x7F); }

/* Case-insensitive substring search (strcasestr equivalent) */
static inline __attribute__((unused))
const char *mm_strcasestr(const char *h, const char *n) {
    uint32_t nl = mm_strlen(n);
    if (!nl) return h;
    for (; *h; h++) {
        if (mm_tolower((unsigned char)*h) == mm_tolower((unsigned char)*n)) {
            uint32_t i;
            for (i = 0; i < nl; i++)
                if (mm_tolower((unsigned char)h[i]) != mm_tolower((unsigned char)n[i]))
                    break;
            if (i == nl) return h;
        }
    }
    return 0;
}

/* Minimal snprintf: supports %d %u %s %c %% only */
#include <stdarg.h>
static inline __attribute__((unused))
int mm_snprintf(char *buf, int sz, const char *fmt, ...)
{
    va_list ap;
    int out = 0;
    va_start(ap, fmt);
#define _EMIT(c) do { if (out < sz-1) buf[out] = (char)(c); out++; } while(0)
    for (; *fmt && (out < sz-1 || out == 0); fmt++) {
        if (*fmt != '%') { _EMIT(*fmt); continue; }
        fmt++;
        if (!*fmt) break;
        if (*fmt == '%') { _EMIT('%'); continue; }
        if (*fmt == 'c') { _EMIT(va_arg(ap, int)); continue; }
        if (*fmt == 's') {
            const char *s = va_arg(ap, const char*);
            if (!s) s = "";
            while (*s) { _EMIT(*s++); }
            continue;
        }
        if (*fmt == 'd' || *fmt == 'i' || *fmt == 'u') {
            char tmp[12]; int len=0, neg=0;
            unsigned n = (unsigned)va_arg(ap, int);
            if (*fmt != 'u' && (int)n < 0) { neg=1; n = (unsigned)(-(int)n); }
            if (n == 0) tmp[len++]='0';
            else { while(n){ tmp[len++]=(char)('0'+n%10); n/=10; } }
            if (neg) _EMIT('-');
            for (int i=len-1; i>=0; i--) _EMIT(tmp[i]);
            continue;
        }
    }
#undef _EMIT
    if (sz > 0) buf[out < sz ? out : sz-1] = '\0';
    va_end(ap);
    return out;
}