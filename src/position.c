#include "coverage.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

/* A count or an index as molto writes them: decimal digits and nothing else.
   strtoul alone would take leading spaces, a sign and trailing garbage. */
static bool parse_count(const char *text, size_t *out) {
    if(text == NULL || text[0] == '\0')
        return false;
    for(const char *c = text; *c != '\0'; c++) {
        if(!isdigit((unsigned char)*c))
            return false;
    }
    errno = 0;
    const unsigned long long value = strtoull(text, NULL, 10);
    if(errno != 0 || value > (size_t)-1)
        return false;
    *out = (size_t)value;
    return true;
}

cov_position cov_position_parse(const char *index, const char *count) {
    const cov_position alone = {.first = true, .last = true, .valid = true};
    if(index == NULL && count == NULL)
        return alone;

    size_t i = 0;
    size_t n = 0;
    if(!parse_count(index, &i) || !parse_count(count, &n) || i == 0 || i > n) {
        cov_position invalid = alone;
        invalid.valid = false;
        return invalid;
    }
    return (cov_position){.first = i == 1, .last = i == n, .valid = true, .index = i, .count = n};
}
