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
//context through the functions
typedef struct {
	const char* json;
	char* stack;
	//use unsigned not char_ptr cause the mem of stack is dynamic,
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

//null / false / true
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
static int cjson_parse_object(cjson_value* v, cjson_context* c);
static int cjson_parse_string(cjson_value* v, cjson_context* c);
static int cjson_parse_array(cjson_value* v, cjson_context*c);
static int cjson_parse_value(cjson_value* v, cjson_context* c)
{
	switch (c->json[0]) {
	case 'n':return cjson_parse_literal(v, c, "null", CJSON_NULL);
	case 'f':return cjson_parse_literal(v, c, "false", CJSON_FALSE);
	case 't':return cjson_parse_literal(v, c, "true", CJSON_TRUE);
	case '\0':return CJSON_PARSE_EXPECT_VALUE;
	case '\"':return cjson_parse_string(v, c);
	case '[':return cjson_parse_array(v, c);
	case '{':return cjson_parse_object(v, c);
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
	//release the memory held inside v
	assert(v != NULL);
	switch (v->type) {
	case CJSON_STRING:
		free(v->s);
		break;
	case CJSON_ARRAY:
		for (unsigned i = 0;i < v->array_size;i++) cjson_free(&v->arr[i]);//recursion
		free(v->arr);
		break;
	case CJSON_OBJECT:
		for (unsigned i = 0;i < v->mem_size;i++) {
			free(v->members[i].key);
			cjson_free(&v->members[i].value);
		}
		free(v->members);
		break;
	default:
		break;
	}
	
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

void set_string_raw(char** dest, const char* s, unsigned len)
{
	*dest = (char*)malloc(len + 1);
	memcpy(*dest, s, len);
	(*dest)[len] = '\0';
}

void set_string(cjson_value* v, const char* s, unsigned len)
{
	assert(v != NULL && (s != NULL || len == 0));
	cjson_free(v);

	/*
	v->s = (char*)malloc(len + 1);
	memcpy(v->s, s, len);
	v->s[len] = '\0';
	*/
	set_string_raw(&(v->s), s, len);
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

static int cjson_parse_string_raw(cjson_context* c)
{
	EXPECT(c, '\"');
	unsigned u;
	const char* p = c->json;
	unsigned old_top = c->top;
	//
	//int ret;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			c->json = p;//skip
			//len = c->top - old_top;
			//set_string(v, (const char*)(cjson_context_pop(c, len)), len);
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

static int cjson_parse_string(cjson_value* v, cjson_context* c)
{
	unsigned old_top = c->top, len;
	int ret = cjson_parse_string_raw(c);
	if (ret == CJSON_PARSE_OK) {
		len = c->top - old_top;
		set_string(v, cjson_context_pop(c, len), len);
	}
	else {
		//c->top = old_top;
	}
	return ret;
}

static int cjson_parse_array(cjson_value* v, cjson_context* c)
{
	EXPECT(c, '[');
	cjson_parse_whitespace(c);
	//
	cjson_value temp_v;//temporarily store the array element in the below loop
	unsigned array_size = 0, old_top = c->top;
	int ret;

	//char *p = c->json;
	for (;;) {
		cjson_parse_whitespace(c);
		//switch (*c->json){
		if (*c->json == '\0') {
			//c->top = old_top;
			ret = CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;//miss the right bracket
			break;
		}
		else if (*c->json == ']') {
			v->type = CJSON_ARRAY;
			v->array_size = array_size;
			if (array_size > 0) {
#ifdef DEBUG
				printf("array size: %d\n", array_size);
#endif // DEBUG
				//naive..... 
				//v->arr = (cjson_value*)cjson_context_pop(c, 
				//	array_size * sizeof(cjson_value)
				//);
				unsigned len = array_size * sizeof(cjson_value);
				v->arr = malloc(len);
				memcpy(v->arr, cjson_context_pop(c, len), len);
			}
			else v->arr = NULL;//remember to init............................................................................
			//c->stack = NULL;
			c->json++;
			return CJSON_PARSE_OK;//done
		}
		else {
			//array element
			//还要考虑ws
			cjson_init(&temp_v);
			ret = cjson_parse_value(&temp_v, c);
			//
			if (CJSON_PARSE_OK == ret) {
				//
				//WARNING: now we should also consider json object like "[1.22KL]", "[1.22K,1]"
				//
				*((cjson_value*)cjson_context_push(c, sizeof(cjson_value))) = temp_v;
				array_size++;
				cjson_parse_whitespace(c);
				//switch (*c->json) {
				if (*c->json == ']')
					continue;//process the next for-loop outside
					//break;
				else if (*c->json == ',') {
					c->json++;
					if (*c->json == ']') {
						//c->top = old_top;
						ret = CJSON_PARSE_INVALID_VALUE;
						break;
						//CJSON_PARSE_ARRAY_EXTRA_COMMA;//ERROR -> extra comma in the end
					}
				}
				//break;
				else {
					//c->top = old_top;
					ret = CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;
					break;
				}
				//}
			}
			else {
				//c->top = old_top;
				break;
				//return ret;
			}
		}
	}
	//release mem
	for (unsigned i = 0;i < array_size;i++)cjson_free((cjson_value*)cjson_context_pop(c, sizeof(cjson_value)));
	c->top = old_top;
	return ret;
}

size_t get_array_size(const cjson_value* v)
{
	assert(v != NULL && v->type == CJSON_ARRAY);
	return v->array_size;
}
//index starts from zero
cjson_value* get_array_element(cjson_value* v, size_t index)
{
	assert(v != NULL && v->type == CJSON_ARRAY);
	assert(index < v->array_size);
	//code like : return &v->arr[0]; doesnt work... //error -> expression must be a pointer to a complete object type
	return &(((cjson_value*)v->arr)[index]);
}

#define OBJECT_ERROR(ERROR_CODE)\
	do{\
	c->top = old_top;\
	free(m.key);\
	m.key = NULL;\
	member* context_mems = (member*)cjson_context_pop(c,mem_size*sizeof(member));\
	for(unsigned i = 0;i<mem_size;i++){\
		free(context_mems[i].key);\
		context_mems[i].key = NULL;\
		cjson_free(&context_mems[i].value);\
		}\
	v->type = CJSON_NULL;\
	return ERROR_CODE;\
	}while(0)
	
static int cjson_parse_object(cjson_value* v, cjson_context* c)
{
	EXPECT(c, '{');
	//unlike the array parsing, we specify the case "{}"
	//it can reduce some complications when parsing the members
	//both are ok
	cjson_parse_whitespace(c);
	if (*c->json == '}') {
		c->json++;
		v->type = CJSON_OBJECT;
		v->members = NULL;
		v->mem_size = 0;
		return CJSON_PARSE_OK;
	}

	int ret;
	unsigned old_top = c->top, len;
	member m;
	unsigned mem_size = 0;
	for (;;) {
		//key part
		cjson_parse_whitespace(c);
		unsigned key_start = c->top;//note here...
		if ((ret = cjson_parse_string_raw(c)) != CJSON_PARSE_OK) {
			OBJECT_ERROR(CJSON_PARSE_OBJECT_MISS_KEY);
		}
		len = c->top - key_start;
		//free(m.key);
		set_string_raw(&m.key, cjson_context_pop(c, len), len);
		m.key_lengh = len;
		cjson_parse_whitespace(c);
		if (*c->json++ != ':') {
			OBJECT_ERROR(CJSON_PARSE_OBJECT_MISS_COLON);
		}// end of key part
		//value part
		cjson_value* mv = &m.value;
		cjson_init(mv);
		cjson_parse_whitespace(c);
		if ((ret = cjson_parse_value(mv, c)) != CJSON_PARSE_OK) {
			OBJECT_ERROR(ret);
		}
		*(member*)cjson_context_push(c, sizeof(member)) = m;
		mem_size++;
		cjson_parse_whitespace(c);
		switch (*c->json) {
		case ',':
			c->json++;
			break;
		case '}':
			c->json++;
			v->type = CJSON_OBJECT;
			len = sizeof(member)*mem_size;
			v->members = malloc(len);
#ifdef DEBUG
			printf("object size is %d\n", mem_size);
#endif // DEBUG
			memcpy(v->members, cjson_context_pop(c, len), len);
			v->mem_size = mem_size;
			return CJSON_PARSE_OK;
		default:
			OBJECT_ERROR(CJSON_PARSE_OBJECT_MISS_COMMA_OR_CURLY_BRACKET);
			//break;
		}
	}
}

size_t get_object_size(const cjson_value* v)
{
	assert(v != NULL && v->type == CJSON_OBJECT);
	return v->mem_size;
}
const char* cjson_get_object_key(const cjson_value* v, size_t index)
{
	assert(v != NULL && v->type == CJSON_OBJECT);
	assert(index < v->mem_size);
	return v->members[index].key;
}
size_t cjson_get_object_key_length(const cjson_value* v, size_t index)
{
	assert(v != NULL && v->type == CJSON_OBJECT);
	assert(index < v->mem_size);
	return strlen(v->members[index].key);
}
cjson_value* cjson_get_object_value(const cjson_value* v, size_t index)
{
	assert(v != NULL && v->type == CJSON_OBJECT);
	assert(index < v->mem_size);
	return &v->members[index].value;
}

cjson_value* get_object_value(const cjson_value* v, const char* key)
{
	assert(v != NULL && v->type == CJSON_OBJECT);
	for (unsigned i = 0;i < v->mem_size;i++) {
		if (strcmp((v->members[i]).key, key) == 0) {
			return &v->members[i].value;
		}
	}
	return NULL;
}