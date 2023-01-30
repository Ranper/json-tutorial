#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
/**
 * @brief 期望c是ch这个字符
 * c 宏定义一般使用do {} while(0),这样无论怎么使用宏定义,基本不会出问题
 */
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

// 指代字符串
typedef struct {
    const char* json;
}lept_context;

/**
 * @brief 跳过c中的空白符号
 * @note 空白符号有 ' ', '\t', '\n', '\r'
 */
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/**
 * @brief 解析null字符
 * 
 * @param c 
 * @param v 
 * @return int 
 */
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value*v){
    EXPECT(c, 't');
    if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e'){
        return LEPT_PARSE_INVALID_VALUE; // 一个无效的true值
    } 
    // 如果确实是true
    c->json += 3; // 跳过当前的指针
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v){
    EXPECT(c, 'f');  // 如果第一个字符不是f,程序员不应该调用这个函数
    if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

/**
 * @brief 解析一个值;作用域只在本文件内(static修饰)
 * 
 * @param c 
 * @param v 
 * @return int 
 */
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_null(c, v);  // value为n开头,表示是null类型,尝试按照null去解析
        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;  //? 这里是为啥呢
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);// 先去掉空白部分
    if ((ret=lept_parse_value(&c , v)) == LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json != '\0'){
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;

}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}
