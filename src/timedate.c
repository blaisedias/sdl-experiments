#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#include "nowplaying.h"

typedef enum {
    TD_WEEKDAY,
    TD_MONTH,
    TD_DAY_OF_MONTH,
    TD_HOUR,
    TD_MINUTES,
    TD_SECONDS,
    TD_YEAR,
    TD_END
}td_field;

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}

#define FREE(x) free_ex((void **)(&x))

static int td_field_spec_to_int(const char* spec) {
    if (0 == strcmp(spec, "weekday")) return TD_WEEKDAY;
    if (0 == strcmp(spec, "month")) return TD_MONTH;
    if (0 == strcmp(spec, "day")) return TD_DAY_OF_MONTH;
    if (0 == strcmp(spec, "hour")) return TD_HOUR;
    if (0 == strcmp(spec, "minutes")) return TD_MINUTES;
    if (0 == strcmp(spec, "seconds")) return TD_SECONDS;
    if (0 == strcmp(spec, "year")) return TD_YEAR;
    return TD_END;
}

void timedate_sprintf(char* buff, size_t bufflen, const char *format) {
    *buff = 0;
    if (NULL == format) {
        return;
    }
    char buf[100];
    time_t now = time(NULL);
    buf[0] = ' ';
    ctime_r(&now, buf+1);

    char*   pre="";
    char*   post;
    char*   pprint = buff;
    size_t  avail = bufflen;
    char*   fmt = strdup(format);
    char*   scan;
    int     wr;

    char*  fields[8] = {
        buf, "", "", "",
        "", "", "", ""
    };
    // buf will have "Wed Jun 24 08:52:24 2026"
    scan = buf;
    for(int ix=0; *scan != 0; ++scan) {
        if (*scan == ' ' || *scan == ':' || *scan == '\n') {
            *scan = 0;
            if (ix < TD_END && *(scan+1))  {
                fields[ix] = scan+1;
                ++ix;
            }
        }
    } 

#define SNPRINTF(str) \
    wr = snprintf(pprint, avail, "%s", (str)); \
    avail -= wr; \
    if (avail < 1) goto END; \
    pprint += wr
    
#define SNPRINTF_STR_FIELD(tok) \
    { \
        const char* str = fields[td_field_spec_to_int(tok)]; \
        if (aw) { \
            char fieldfmt[16]; \
            snprintf(fieldfmt, sizeof(fieldfmt), "%%%ss", aw); \
            wr = snprintf(pprint, avail, fieldfmt, (str)); \
        } else { \
            wr = snprintf(pprint, avail, "%s", (str)); \
        } \
        avail -= wr; \
        if (avail < 1) goto END; \
        pprint += wr; \
    }

    scan = fmt;
    while(*scan) {
        pre = scan;
        while(*scan && *scan != '[' && *scan != '{') {
            ++scan;
        }
        if (*scan == 0) goto END;
/*        
        if (*scan == '[') {
            *scan = '\0';
            SNPRINTF(pre);
            ++scan;
            pre = scan;
            while(*scan && *scan != '{') {
                ++scan;
            }
            if (*scan == 0) goto END;
            *scan='\0';
            ++scan;
            char *tok = scan;
            char *aw = NULL;
            while(*scan && *scan != '}') {
                if (*scan == ':') {
                    aw = scan + 1;
                    *scan = '\0';
                }
                ++scan;
            }
            if (*scan == 0) goto END;
            *scan = '\0';
            ++scan;
            post=scan;
            while(*scan && *scan != ']') {
                ++scan;
            }
            if (*scan == 0) goto END;
            *scan = '\0';
            
            SNPRINTF(pre);
            SNPRINTF_STR_FIELD(tok);
            SNPRINTF(post);
            pre = post = scan; // pre and post point to empty strings
            ++scan;
        } else if (*scan == '{') {
*/
        if (*scan == '{') {
            post = scan;
            *scan = '\0';
            ++scan;
            char *tok = scan;
            char *aw = NULL;
            while(*scan && *scan != '}') {
                if (*scan == ':') {
                    aw = scan + 1;
                    *scan = '\0';
                }
                ++scan;
            }
            if (*scan == 0) goto END;
            *scan = '\0';
            SNPRINTF(pre);
            SNPRINTF_STR_FIELD(tok);
            pre = post = scan; // pre and post point to empty strings
            ++scan;
        }
    }
END:
    SNPRINTF(pre);
    FREE(fmt);
#undef SNPRINTF
#undef SNPRINTF_STR_FIELD
}
