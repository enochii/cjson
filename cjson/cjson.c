#define _CRTDBG_MAP_ALLOC  
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>  
#include <crtdbg.h>

#ifndef CJSON_PARSE_STACK_INIT_SIZE
#define CJSON_PARSE_STACK_INIT_SIZE 256
#endif // CJSON_PARSE_STACK_INIT_SIZE


#include "cjson.h"
#include <assert.h>
#include <stdlib.h>//为什么一开始会include cstdlib...
#include <string.h>
#include <math.h>
#include <stdio.h>

#define DEBUG

//note that the asserted expression should not have side effect, 
//which will make difference between debug and release
#define EXPECT(c, ch) do{assert(*c->json == (ch));c->json++;}while(0)

#define ISDIGIT(ch) ((ch) >= '0'&& (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

//copy the pointer to be able to increment the ptr when pass the json
//context through the function
typedef struct {
	const char* json;
	char* stack;
	//use unsigned not char_ptr cause the mem of stack is dynamic
	//when realloc new mem the top will be invalid
	size_t size, top;
} cjson_context;

//all type
#define ESCAPE_DIGIT(p) while (ISDIGIT(*p))++p
static int cjson_parse_number(cjson_value*v, cjson_context* c)
{
	const char* p = c->json;
	if (!ISDIGIT(*p)&&*p!='-'){ return CJSON_PARSE_INVALID_VALUE; }
	if (*p == '-') { ++p; }//skip symbol '-'
	if(*p == '0'){//leading 0
		//avoid number like: 00, 0x1
		++p;//skip
		//. or nothing
		//not_singular should not be handled here...
		//if (*p != '.'&&*p != '\0') { return CJSON_PARSE_INVALID_VALUE; }
	}
	else {
		//avoid number like: *digit.(nothing)
		ESCAPE_DIGIT(p);
	}
	//handle frac if exist
	if (*p == '.') {
		++p;//skip
		if (!ISDIGIT(*p))return CJSON_PARSE_INVALID_VALUE;
		ESCAPE_DIGIT(p);
	}
	//handle exp
	if (*p == 'e' || *p == 'E') {
		++p;//skip
		if (*p == '+' || *p == '-') { ++p; }//skip
		if (*p == '\0')return CJSON_PARSE_INVALID_VALUE;
		ESCAPE_DIGIT(p);
	}
	//no need
	//if (*p != '\0')return CJSON_PARSE_NOT_SINGULAR;
	//char* end = NULL;
	//
	v->n = strtod(c->json, NULL);
	//handle (-)infinity
	if (fabs(v->n) == HUGE_VAL)return CJSON_PARSE_NUMBER_TOO_BIG;
	
	//if (end == c->json) { return CJSON_PARSE_INVALID_VALUE; }
	
	v->type = CJSON_NUMBER;
	c->json = p;
	return CJSON_PARSE_OK;
}

static void cjson_parse_whitespace(cjson_context* c)
{
	const char* p = c->json;
	while (*p == '\n' || *p == '\t' || *p == ' '||*p=='\r') {
		p++;
	}

	c->json = p;
}

//old codes
/*
static int cjson_parse_null(cjson_value* v, cjson_context* c)
{
	EXPECT(c, 'n');

	if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
		return CJSON_PARRSE_INVALID_VALUE;

	c->json += 3;
	v->type = CJSON_NULL;
	return CJSON_PARSE_OK;
}


static int cjson_parse_false(cjson_value* v, cjson_context* c)
{
	EXPECT(c, 'f');

	const char* p = c->json;
	if (p[0] != 'a' || p[1] != 'l' || p[2] != 's' || p[3] != 'e') {
		return CJSON_PARRSE_INVALID_VALUE;
	}
	c->json += 4;
	v->type = CJSON_FALSE;
	return CJSON_PARSE_OK;
}

static int cjson_parse_true(cjson_value* v, cjson_context *c)
{
	EXPECT(c, 't');

	const char *p = c->json;
	if (p[0] != 'r' || p[1] != 'u' || p[2] != 'e') {
		return CJSON_PARRSE_INVALID_VALUE;
	}

	c->json += 3;
	v->type = CJSON_TRUE;
	return CJSON_PARSE_OK;
}
*/

static int cjson_parse_literal(cjson_value* v, cjson_context* c, 
	const char* literal_str, cjson_type literal_type)//generalize
{	
	assert(literal_str != NULL);
	EXPECT(c, *literal_str);
	
	const char *p = c->json;
	size_t len = strlen(literal_str);
	for (size_t i = 0;i < len - 1;++i) {
		if (p[i] != literal_str[i + 1]) { return CJSON_PARSE_INVALID_VALUE; }
	}
	c->json += len - 1;
	v->type = literal_type;
	return CJSON_PARSE_OK;
}
//
static int cjson_parse_string(cjson_value* v, cjson_context* c);
static int cjson_parse_value(cjson_value* v, cjson_context* c)
{
	switch (c->json[0]) {
	case 'n':return cjson_parse_literal(v, c, "null", CJSON_NULL);
	case 'f':return cjson_parse_literal(v, c, "false", CJSON_FALSE);
	case 't':return cjson_parse_literal(v, c, "true", CJSON_TRUE);
	case '\0':return CJSON_PARSE_EXPECT_VALUE;
	case '\"':return cjson_parse_string(v, c);
	default:return cjson_parse_number(v, c);
	}
}

int cjson_parse(cjson_value* v, const char* json)
{
	assert(json != NULL);

	//maintain the state of the ptr
	cjson_context context;
	context.json = json;
	//init stack
	context.stack = NULL;
	context.size = context.top = 0;

	//fail logo
	v->type = CJSON_NULL;
	
	cjson_parse_whitespace(&context);
	int retval= cjson_parse_value(v, &context);
	
	if (retval == CJSON_PARSE_OK) {
		cjson_parse_whitespace(&context);
		if (context.json[0] != '\0') {
			v->type = CJSON_NULL;//emmm, remember this
			retval = CJSON_PARSE_NOT_SINGULAR;
		}
	}
	//free
	assert(context.top == 0);
	free(context.stack);
	return retval;
}

//这里没加const居然没事耶...
int get_type(const cjson_value* v)
{
	assert(v != NULL);
	return v->type;
}

double get_number_value(const cjson_value* v)
{
	assert(v != NULL && v->type == CJSON_NUMBER);
	return v->n;
}

void set_number_value(cjson_value* v, double n)
{
	assert(v != NULL);
	cjson_free(v);
	v->n = n;
	v->type = CJSON_NUMBER;
}

void cjson_free(cjson_value* v)
{
	assert(v != NULL);
	if (v->type == CJSON_STRING) { free(v->s); }
	v->type = CJSON_NULL;
}

int get_boolean(const cjson_value* v)
{
	assert(v != NULL && (v->type == CJSON_TRUE || v->type == CJSON_FALSE));
	return v->type;
}
void set_boolean(cjson_value* v, int b)//b = 0 or 1
{
	assert(v != NULL);
	cjson_free(v);
	v->type = b ? CJSON_TRUE:CJSON_FALSE;
}

const char* get_string(const cjson_value* v)
{
	assert(v != NULL && v->type == CJSON_STRING);
	return v->s;
}
unsigned get_string_length(const cjson_value* v)
{
	assert(v != NULL && v->type == CJSON_STRING);
	return v->len;
}

void set_string(cjson_value* v, const char* s, unsigned len)
{
	assert(v != NULL && (s != NULL || len == 0));
	cjson_free(v);

	v->s = (char*)malloc(len + 1);
	memcpy(v->s, s, len);
	v->s[len] = '\0';
	v->len = len;
	v->type = CJSON_STRING;
}

static void* cjson_context_push(cjson_context* c, unsigned size)
{
	assert(c != NULL && size > 0);
	void* ret = NULL;
	//>= / >
	//top represent the current len of str
	if (c->top + size >= c->size) {
		if (c->size == 0) { c->size = CJSON_PARSE_STACK_INIT_SIZE; }
		while(c->top+size>c->size){
			c->size += c->size >> 1;/* c->size * 1.5 */
		}
		c->stack = (char*)realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* cjson_context_pop(cjson_context* c, unsigned size)
{
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

#define PUTC(c, ch) *(char*)(cjson_context_push((c), sizeof(char))) = ch

/*
//i don't think return exp in macro is good...
#define HANDLE_ESCAPE(c, p) \
	do{\
			char ch = *p;\
			switch(ch){\
				case 'n':ch = '\n';break;\
				case 'r':ch = '\r';break;\
				case 'b':ch = '\b';break;\
				case 'f':ch = '\f';break;\
				case 't':ch = '\t';break;\
				case '\\':\
				case '/':\
				case '\"':\
					break;\
				default: \
					c->top = old_top;\
					return CJSON_PARSE_INVALID_STRING_ESCAPE;\
			}\
			PUTC(c, ch);\
	}while(0)
*/

#define ISHEX(ch) \
((ISDIGIT(ch)) || (ch>='a'&&ch<='f') || (ch>='A'&&ch<='F'))
#define ISHEX4(p) \
do{\
	for (int i = 0;i < 4;++i) {\
		char ch = *(p + i);\
		if (!ISHEX(ch))return NULL;}\
	}while(0)
//string implementation
static const char* cjson_parse_hex4(const char* p, unsigned* u)
{
	ISHEX4(p);
	//here the 4 seems to only restrict the max read byte...
	//so "12\\" won't raise a return error
	int ret = sscanf_s(p, "%4x", u);
	if (ret == EOF || ret <= 0)return NULL;

	p += 4;
	//move this logic outside have two reasons:
	// 1.we can handle the invalid suggorate
	// 2.we won't "repeat ourselves", that's to say, we won't do the parse hex4 again
	/*
	if (*u >= 0xd800 && *u <= 0xdbff) {
		//u is now the high surrogate, next we should parse the low
		unsigned L = 0;
		p += 2;//    skip  \\u
		sscanf_s(p, "%4x", &L);
#ifdef DEBUG
		//printf("\n%x\n%x\n", *u, L);
#endif // DEBUG
		*u = 0x10000 + (*u - 0xD800) * 0x400 + (L - 0xDC00);
		p += 4;
	}
	*/

#ifdef DEBUG
	printf("\nthe unsigned value is %x\n", *u);
#endif // DEBUG

	return p;
}

//this macro is unneccessary...
#define OutputByte(u) sprintf(cjson_context_push(c, sizeof(char)),"%c",u)

static void cjson_encode_utf8(cjson_context *c, unsigned u)
{
	assert(u >= 0x0000 && u <= 0x10FFFF);
	/* \ TODO */
	if (u >= 0x0000 && u <= 0x007F) {
		//OutputByte(0x00 | (u				& 0x7F));
		PUTC(c, (0x00 | (u				& 0x7F)));
	}
	else if (u >= 0x0080 && u <= 0x07FF) {
		PUTC(c, 0xC0 | ((u >> 6 ) & 0x1F));
		PUTC(c, 0x80 | (u				& 0x3F));
	}
	else if (u >= 0x800 && u <= 0xFFFF) {
		PUTC(c, 0xE0 | ((u >> 12) & 0x0F));
		PUTC(c, 0x80 | ((u >> 6 ) & 0x3F));
		PUTC(c, 0x80 | (u				& 0x3F));
	}
	else if (u >= 0x10000 && u <= 0x10FFFF) {
		PUTC(c, 0xF0 | ((u >> 18) & 0x07));
		PUTC(c, 0x80 | ((u >> 12) & 0x3F));
		PUTC(c, 0x80 | ((u >> 6 ) & 0x3F));
		PUTC(c, 0x80 | (u				& 0x3F));
	}
}

#define STRING_ERROR(ret) do{c->top = old_top;return ret;}while(0)

static int cjson_parse_string(cjson_value* v, cjson_context* c)
{
	EXPECT(c, '\"');
	unsigned u;
	const char* p = c->json;
	unsigned old_top = c->top, len;
	//
	//int ret;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			c->json = p;//skip
			len = c->top - old_top;
			set_string(v, (const char*)(cjson_context_pop(c, len)), len);
			return CJSON_PARSE_OK;
		case '\0':
			c->top = old_top;//roll back
			return CJSON_PARSE_MISS_QUOTATION;

			//转义字符
		case '\\':
		{
			//char ch = *p;
				switch (*p++) {//p++可以省去对\u中得再skip一次u
					
				case 'n':PUTC(c, '\n');;break;
				case 'r':PUTC(c, '\r');break;
				case 'b':PUTC(c, '\b');break;
				case 'f':PUTC(c, '\f');break;
				case 't':PUTC(c, '\t');break;
				case '\\':PUTC(c, '\\');break;
				case '/':PUTC(c, '/');break;
				case '\"':PUTC(c, '\"');break;
						//break;
				case 'u':
					/*1.解析4位十六进制整数为码点
					   2.把码点编码成utf-8
					*/
					if (!(p = cjson_parse_hex4(p, &u)))
						STRING_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX);
					/* \ TODO surrogate handling */
					//判断码点是否在范围内
					if (u >= 0xd800 && u <= 0xdbff) {
						//u is now the high surrogate, next we should parse the low
						int H = u;
						if (
							*p++!='\\'||
							*p++!='u'||
							!(p = cjson_parse_hex4(p, &u))||
							!(u>=0xDC00&&u<=0xDFFF)
							)
							STRING_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE);
						u = 0x10000 + (H - 0xD800) * 0x400 + (u - 0xDC00);
					}
					cjson_encode_utf8(c, u);
					break;
				default: 
						c->top = old_top;
						return CJSON_PARSE_INVALID_STRING_ESCAPE;
				}
				
			//p++;
			break;
		}
		default:
			if ((unsigned)ch < 0x20) {
				c->top = old_top;
				return CJSON_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}