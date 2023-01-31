#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <stdio.h> /* printf*/
#include <errno.h> /*for errno*/
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         (ch >= '0' && ch <= '9')
#define ISDIGIT1TO9(ch)     (ch >= '1' && ch <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/*
// static int lept_parse_true(lept_context* c, lept_value* v) {
//     EXPECT(c, 't');
//     if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_TRUE;
//     return LEPT_PARSE_OK;
// }

// static int lept_parse_false(lept_context* c, lept_value* v) {
//     EXPECT(c, 'f');
//     if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 4;
//     v->type = LEPT_FALSE;
//     return LEPT_PARSE_OK;
// }

// static int lept_parse_null(lept_context* c, lept_value* v) {
//     EXPECT(c, 'n');
//     if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
//         return LEPT_PARSE_INVALID_VALUE;
//     c->json += 3;
//     v->type = LEPT_NULL;
//     return LEPT_PARSE_OK;
// }
*/

// how to use lept_parse_literal(c, v, "null", LEPT_NULL);
static int lept_parse_literal(lept_context* c, lept_value* v, const char* type_str, lept_type value_type){
    EXPECT(c, type_str[0]);
    size_t i = 0; // size_t 索引最好用size_t 类型
    for( ; type_str[i+1]!= '\0' ;i++){
        if (c->json[i] != type_str[i+1]){
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    c->json += i;
    v->type = value_type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    // char* end;
    /* \TODO validate number */
    
    const char *p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else{
        if (ISDIGIT1TO9(*p)){
            for(p++; ISDIGIT(*p); p++);
        }else {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    if (*p == '.'){
        p++;
        if (!ISDIGIT(*p)){  // 这里判断过一次了,后面就必要再对*p判断了
            return LEPT_PARSE_INVALID_VALUE;
        }
        for(p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E'){
        p++;
        if(*p == '+' || *p == '-')  p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)){
        // printf("errno=%d, v->n=%lf\n", errno, v->n);
        return LEPT_PARSE_NUMBER_TOO_BIG;  // 过大
    }

    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v){
    switch (*c->json)
    {
        case 't':   return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':   return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':   return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:    return lept_parse_number(c, v);
        case '\0':  return LEPT_PARSE_EXPECT_VALUE;
    }

}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
