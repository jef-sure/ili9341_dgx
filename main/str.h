/*
 * str.h
 *
 *  Created on: Mar 16, 2021
 *      Author: anton
 */

#ifndef _STR_H_
#define _STR_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct _str_t {
    size_t capacity;
    size_t length;
    char data[1];
} str_t;

typedef struct _str_vector_t {
    size_t length;
    str_t **vector;
} str_vector_t;

static inline size_t _str_allign(size_t ln) {
    ln += 4 - ((ln + sizeof(str_t)) & 3);
    return ln;
}

static const size_t str_npos = (size_t)(-1);
static const str_t str_empty = { 1u, 0u, { 0 } };

static inline str_t* str_new_ln(size_t ln) {
    str_t *ret;
    size_t cap = _str_allign(ln);
    ret = (str_t*) malloc(sizeof(str_t) + cap);
    assert(ret);
    ret->length = ln;
    ret->capacity = cap;
    return ret;
}

static inline str_t* str_new_pc(const char *pc) {
    size_t sln = strlen(pc);
    str_t *ret = str_new_ln(sln);
    memcpy(ret->data, pc, sln);
    return ret;
}

static inline str_t* str_new_pcln(const char *pc, size_t ln) {
    str_t *ret = str_new_ln(ln);
    memcpy(ret->data, pc, ln);
    return ret;
}

static inline str_t* str_new_str(const str_t *str) {
    return str_new_pcln(str->data, str->length);
}

static inline void str_destroy(str_t **dst) {
    free(*dst);
    *dst = NULL;
}

static inline void str_clear(str_t *str) {
    str->length = 0;
}

static inline char* str_c(str_t *str) {
    str->data[str->length] = 0;
    return str->data;
}

static inline size_t str_length(const str_t *str) {
    if (str != NULL) {
        return str->length;
    } else {
        return 0;
    }
}

static inline bool str_is_empty(const str_t *str) {
    size_t r;
    if (str_length(str) == 0) {
        return 1;
    }
    for (r = 0; r < str_length(str) && isspace(str->data[r]); ++r);
    if (r == str->length) {
        return 1; // true
    }
    return 0; // false
}

static inline bool str_xeq_pc(const str_t *str, const char *pc) {
    size_t sln = strlen(pc);
    if (str->length < sln) {
        return 0; // false
    }
    return memcmp(str->data + (str->length - sln), pc, sln) == 0;
}

static inline str_t* str_substr(const str_t *str, size_t pos, size_t len) {
    if (pos >= str->length || len == 0) {
        return str_new_ln(0);
    }
    if (len == str_npos || pos + len > str->length) {
        len = str->length - pos;
    }
    return str_new_pcln(str->data + pos, len);
}

static inline void str_alltrim(str_t **dst_str) {
    size_t p1, p2;
    str_t *tmp;
    for (p1 = 0; p1 < (*dst_str)->length && isspace((*dst_str)->data[p1]); ++p1);
    if (p1 == (*dst_str)->length) {
        (*dst_str)->length = 0;
        return;
    }
    for (p2 = (*dst_str)->length - 1; p2 + 1 > p1 && isspace((*dst_str)->data[p2]); --p2);
    tmp = str_substr(*dst_str, p1, p2 - p1 + 1);
    str_destroy(dst_str);
    *dst_str = tmp;
}

static inline void str_chomp(str_t *str) {
    char c;
    if (str != NULL) while (str->length != 0 && ((c = str->data[str->length - 1]) == '\r' || c == '\n'))
        --str->length;
}

static inline void str_tolower(str_t *str) {
    size_t i;
    for (i = 0; i < str->length; ++i)
        str->data[i] = tolower(str->data[i]);
}

static inline void str_toupper(str_t *str) {
    size_t i;
    for (i = 0; i < str->length; ++i)
        str->data[i] = toupper(str->data[i]);
}

static inline void str_append_str(str_t **dest_str, const str_t *str) {
    if (str == NULL || str->length == 0) {
        return;
    }
    if (*dest_str == NULL)
        *dest_str = str_new_str(str);
    else {
        if ((*dest_str)->capacity < (*dest_str)->length + str->length + 1) {
            str_t *tmp = str_new_ln((*dest_str)->capacity + (str->capacity * 2));
            memcpy(tmp->data, (*dest_str)->data, (*dest_str)->length);
            tmp->length = (*dest_str)->length;
            free(*dest_str);
            *dest_str = tmp;
        }
        memcpy((*dest_str)->data + (*dest_str)->length, str->data, str->length);
        (*dest_str)->length += str->length;
    }
}

static inline void str_append_pc(str_t **dest_str, const char *pc) {
    size_t sln;
    if (pc == NULL || (sln = strlen(pc)) == 0) {
        return;
    }
    if (*dest_str == NULL)
        *dest_str = str_new_pc(pc);
    else {
        if ((*dest_str)->capacity < (*dest_str)->length + sln + 1) {
            str_t *tmp = str_new_ln((*dest_str)->capacity + (sln * 2));
            memcpy(tmp->data, (*dest_str)->data, (*dest_str)->length);
            tmp->length = (*dest_str)->length;
            free(*dest_str);
            *dest_str = tmp;
        }
        memcpy((*dest_str)->data + (*dest_str)->length, pc, sln);
        (*dest_str)->length += sln;
    }
}

static inline void str_append_pcln(str_t **dest_str, const char *pc, size_t sln) {
    if (pc == NULL || sln == 0) {
        return;
    }
    if (*dest_str == NULL)
        *dest_str = str_new_pcln(pc, sln);
    else {
        if ((*dest_str)->capacity < (*dest_str)->length + sln + 1) {
            str_t *tmp = str_new_ln((*dest_str)->capacity + (sln * 2));
            memcpy(tmp->data, (*dest_str)->data, (*dest_str)->length);
            tmp->length = (*dest_str)->length;
            free(*dest_str);
            *dest_str = tmp;
        }
        memcpy((*dest_str)->data + (*dest_str)->length, pc, sln);
        (*dest_str)->length += sln;
    }
}

static inline void str_append_c(str_t **dest_str, char c) {
    if (*dest_str == NULL)
        *dest_str = str_new_pcln(&c, 1);
    else {
        if ((*dest_str)->capacity < (*dest_str)->length + 2) {
            str_t *tmp = str_new_ln((*dest_str)->capacity + 16);
            memcpy(tmp->data, (*dest_str)->data, (*dest_str)->length);
            tmp->length = (*dest_str)->length;
            free(*dest_str);
            *dest_str = tmp;
        }
        (*dest_str)->data[(*dest_str)->length] = c;
        (*dest_str)->length += 1;
    }
}

static inline void str_replace_str(str_t **dest_str, size_t pos, size_t n, const str_t *str) {
    size_t nl;
    str_t *tmp;
    if (pos == (*dest_str)->length) {
        str_append_str(dest_str, str);
        return;
    }
    if (pos > (*dest_str)->length) {
        return;
    }
    if (str == NULL) str = &str_empty;
    if (n == str_npos || pos + n > (*dest_str)->length) n = (*dest_str)->length - pos;
    nl = (*dest_str)->length + str->length - n;
    tmp = str_new_ln(nl);
    if (pos) memcpy(tmp->data, (*dest_str)->data, pos);
    if (str->length) memcpy(tmp->data + pos, str->data, str->length);
    if (pos + n < (*dest_str)->length)
        memcpy(tmp->data + pos + str->length, (*dest_str)->data + pos + n, (*dest_str)->length - n - pos);
    str_destroy(dest_str);
    *dest_str = tmp;
}

static inline size_t str_find_str(const str_t *str, const str_t *substr, size_t pos) {
    if (str == NULL) return str_npos;
    if (substr->length > str->length || str->length == 0 || substr->length == 0) {
        return str_npos;
    }
    while (pos + substr->length <= str->length)
        if (memcmp(str->data + pos, substr->data, substr->length) == 0) {
            return pos;
        } else
            ++pos;
    return str_npos;
}

static inline size_t str_find_pc(const str_t *str, const char *pc, size_t pos) {
    size_t sln = strlen(pc);
    if (str == NULL) return str_npos;
    if (sln > str->length || str->length == 0 || sln == 0) {
        return str_npos;
    }
    while (pos + sln <= str->length)
        if (memcmp(str->data + pos, pc, sln) == 0) {
            return pos;
        } else
            ++pos;
    return str_npos;
}

static inline size_t str_find_c(const str_t *str, char c, size_t pos) {
    char *cptr;
    if (str == NULL) return str_npos;
    if (pos >= str->length || str->length == 0) {
        return str_npos;
    }
    cptr = (char*) memchr(str->data + pos, c, str->length - pos);
    if (cptr != NULL) {
        return cptr - str->data;
    }
    return str_npos;
}

static inline size_t str_rfind_c(const str_t *str, char c, size_t pos) {
    if (str == NULL) return str_npos;
    if (str->length == 0) {
        return str_npos;
    }
    if (pos >= str->length) pos = str->length - 1;
    while (pos + 1 > 0) {
        if (str->data[pos] == c) {
            return pos;
        }
        --pos;
    }
    return str_npos;
}

static inline size_t str_rfind_str(const str_t *str, const str_t *substr, size_t pos) {
    if (str == NULL) return str_npos;
    if (substr->length > str->length || str->length == 0 || substr->length == 0) {
        return str_npos;
    }
    if (pos == str_npos || pos + substr->length > str->length) pos = str->length - substr->length;
    while (pos + 1 > 0)
        if (memcmp(str->data + pos, substr->data, substr->length) == 0) {
            return pos;
        } else
            --pos;
    return str_npos;
}

static inline size_t str_rfind_pc(const str_t *str, const char *pc, size_t pos) {
    size_t sln = strlen(pc);
    if (str == NULL) return str_npos;
    if (sln > str->length || str->length == 0 || sln == 0) {
        return str_npos;
    }
    if (pos == str_npos || pos + sln > str->length) pos = str->length - sln;
    while (pos + 1 > 0)
        if (memcmp(str->data + pos, pc, sln) == 0) {
            return pos;
        } else
            --pos;
    return str_npos;
}

static inline str_vector_t* str_vector_new(size_t ln) {
    str_vector_t *ret = (str_vector_t*) malloc(sizeof(*ret));
    assert(ret);
    ret->vector = (str_t**) calloc(ln, sizeof(str_t*));
    ret->length = ln;
    return ret;
}

static inline void str_vector_add_str(str_vector_t *strv, str_t *str) {
    strv->vector = (str_t**) realloc(strv->vector, (strv->length + 1) * sizeof(str_t*));
    strv->vector[strv->length] = str;
    ++strv->length;
}

static inline void str_vector_del_str(str_vector_t *strv) {
    str_destroy(strv->vector + strv->length - 1);
    --strv->length;
}

static inline void str_vector_destroy(str_vector_t **pstrv) {
    if (*pstrv != NULL) {
        size_t n;
        for (n = 0; n < (*pstrv)->length; ++n)
            str_destroy((*pstrv)->vector + n);
        free((*pstrv)->vector);
        free(*pstrv);
        *pstrv = NULL;
    }
}

static inline str_vector_t* str_split_pc(const str_t *str, const char *pc) {
    size_t sln;
    str_vector_t *ret = str_vector_new(0);
    if (str_length(str) == 0) {
        str_vector_add_str(ret, str_new_ln(0));
        return ret;
    }
    if (pc == NULL || (sln = strlen(pc)) == 0) {
        size_t i;
        str_vector_destroy(&ret);
        ret = str_vector_new(str->length);
        for (i = 0; i < str->length; ++i)
            ret->vector[i] = str_new_pcln(str->data + i, 1);
        return ret;
    } else {
        size_t npos = 0, fpos;
        do {
            fpos = str_find_pc(str, pc, npos);
            if (fpos != str_npos) {
                str_vector_add_str(ret, str_substr(str, npos, fpos - npos));
            } else {
                str_vector_add_str(ret, str_substr(str, npos, str->length - npos));
                break;
            }
            npos = fpos + sln;
        } while (1);
        return ret;
    }
}

#endif /* _STR_H_ */
