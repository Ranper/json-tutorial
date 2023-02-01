#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;

static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* p = c->json;
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');  // 如果是字符串,第一个字符期望是 "
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"': // 另外一个引号,表示结束
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':  // 还没收到引号就收到了 \0 表示字符串结束,表示缺少引号标志
                c->top = head;  // 栈的指针回退
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            default:  // 如果是其他字符,直接压栈
                if ((unsigned char)ch < 0x20){
                    c->top = head;
                    return LEPT_PARSE_INVALID_STRING_CHAR; // 无效的string字符
                }
                PUTC(c, ch);
                break;
            case '\\':  // 如果是转义字符,直接跳过
                // char escaped_ch = *p++;
                switch (*p++){
                case '\"':  PUTC(c, '\"');  break;
                case '\\':  PUTC(c, '\\');  break;
                case '/':   PUTC(c, '/');   break;
                case 'b':   PUTC(c, '\b');  break;
                case 'f':   PUTC(c, '\f');  break;
                case 'n':   PUTC(c, '\n');  break;
                case 'r':   PUTC(c, '\r');  break;
                case 't':   PUTC(c, '\t');  break;
                default:    
                    c->top = head; // 出错了,记得回退栈顶
                    return LEPT_PARSE_INVALID_STRING_ESCAPE; // 表示这是一个无效的转义字符
                }
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);  // default 的位置没有要求.长知识了
        case '"':  return lept_parse_string(c, v);  // 这里为什么不需要转义啦;ans:因为c语言里面的”不需要转义,json里面的“需要转义
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);  // 因为c.stack是为了解析字符串暂存的,所以解析之后里面应该是空的.
    free(c.stack);
    return ret;
}

// 实际上是释放v中通过malloc分配的内存
void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

// 下列函数是一系列各个类型的get和set函数

// 返回类型是true or false, 所以直接跟lept_true比较,隐藏中间变量
int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v -> type == LEPT_FALSE || v -> type == LEPT_TRUE));
    /* \TODO */
    return v->type == LEPT_TRUE;
}

// set的含义是把一个lept_value类型设置为一个具体的值
void lept_set_boolean(lept_value* v, int b) {
    /* \TODO */
    assert(v != NULL);
    lept_free(v); /*考虑到之前v可能用于其他的类型,所以在改赋值为boolean前要先free掉*/
    v -> type = b == 0 ? LEPT_FALSE : LEPT_TRUE; 
    return;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    /* \TODO */
    assert(v != NULL);
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
    return;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0)); // s不能为空字符串,如果s为空字符串时,指定的长度必须为0
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
