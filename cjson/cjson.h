#ifndef CJSON_H_
#define CJSON_H_

#include <stddef.h> /* size_t */

typedef enum {
	CJSON_NULL = 0,
	CJSON_TRUE,
	CJSON_FALSE,
	CJSON_NUMBER,
	CJSON_STRING,
	CJSON_ARRAY,
	CJSON_OBJECT,
}cjson_type;

enum {
	CJSON_PARSE_OK = 0,
	CJSON_PARSE_EXPECT_VALUE,//暂时不懂这里的expect啥意思
	CJSON_PARSE_INVALID_VALUE,
	CJSON_PARSE_NOT_SINGULAR,
	CJSON_PARSE_NUMBER_TOO_BIG,
	CJSON_PARSE_MISS_QUOTATION,
	CJSON_PARSE_INVALID_STRING_ESCAPE,
	CJSON_PARSE_INVALID_STRING_CHAR,
	CJSON_PARSE_INVALID_UNICODE_HEX,
	CJSON_PARSE_INVALID_UNICODE_SURROGATE
};

typedef struct {
	cjson_type type;
	union {
		struct { char* s;size_t len; };//
		double n;//存储number的数值
	};
}cjson_value;

//cjson_value由用户提供
int cjson_parse(cjson_value* v, const char* json);

cjson_type get_type(const cjson_value* v);

double get_number_value(const cjson_value* v);
void set_number_value(cjson_value* v, double n);

void cjson_free(cjson_value* v);

#define cjson_init(v) do{(v)->type = CJSON_NULL;}while(0)
#define cjson_set_null(v) cjson_free(v)

int get_boolean(const cjson_value* v);
void set_boolean(cjson_value* v, int b);

const char* get_string(const cjson_value* v);
unsigned get_string_length(const cjson_value* v);
void set_string(cjson_value* v, const char* s, unsigned len);

#endif	/*CJSON_H_*/