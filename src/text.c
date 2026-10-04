#include "coverage.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool reserve(cov_text *text, size_t extra) {
    if(text->failed)
        return false;
    const size_t needed = text->length + extra + 1;
    if(needed <= text->capacity)
        return true;
    size_t next = text->capacity == 0 ? 256 : text->capacity;
    while(next < needed)
        next *= 2;
    char *grown = realloc(text->data, next);
    if(grown == NULL) {
        text->failed = true;
        return false;
    }
    text->data = grown;
    text->capacity = next;
    return true;
}

void cov_text_add(cov_text *text, const char *piece) {
    const size_t length = strlen(piece);
    if(!reserve(text, length))
        return;
    memcpy(text->data + text->length, piece, length + 1);
    text->length += length;
}

void cov_text_addf(cov_text *text, const char *format, ...) {
    va_list args;
    va_start(args, format);
    va_list again;
    va_copy(again, args);
    const int needed = vsnprintf(NULL, 0, format, args);
    va_end(args);
    if(needed < 0 || !reserve(text, (size_t)needed)) {
        text->failed = true;
        va_end(again);
        return;
    }
    vsnprintf(text->data + text->length, (size_t)needed + 1, format, again);
    va_end(again);
    text->length += (size_t)needed;
}

char *cov_text_take(cov_text *text) {
    if(text->failed || !reserve(text, 0)) {
        free(text->data);
        *text = (cov_text){0};
        return NULL;
    }
    text->data[text->length] = '\0';
    char *taken = text->data;
    *text = (cov_text){0};
    return taken;
}
