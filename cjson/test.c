#define _CRTDBG_MAP_ALLOC  
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>  
//#include <crtdbg.h>

#include <stdio.h>
#include "cjson.h"
#include <assert.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;



#define EXPECT_EQ_BASE(equality, expect, actual, format) \
		do{\
				test_count++;\
				if (equality){\
					test_pass++;\
					}\
				else {\
						fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", \
							__FILE__, __LINE__, expect, actual);\
					main_ret=1;\
				}\
		}while(0)

#define EXPECT_EQ_STRING(expect, actual, len)\
		do{\
				int flag = 1;\
				for(unsigned i = 0;i<len;++i){\
					if(expect[i] != actual[i])\
						flag = 0;\
				}\
				EXPECT_EQ_BASE(flag, expect,actual,"%s");\
		}while(0)

#define EXPECT_EQ_INT(expect, actual)\
		EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual)\
		EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%lf")
#define TEST_ERROR(error, json)\
	do{\
			cjson_value v;\
			v.type = CJSON_FALSE;\
			EXPECT_EQ_INT(error, (cjson_parse(&v, json)));\
			EXPECT_EQ_INT(CJSON_NULL, get_type(&v));\
			if(get_type(&v)==CJSON_STRING)printf("%s\n", get_string(&v));/*if(error==CJSON_PARSE_NUMBER_TOO_BIG)printf("%lf",v.n);*/\
	}while(0)\

#define TEST_STRING(expect, json)\
    do {\
        cjson_value v;\
        cjson_init(&v);\
        EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, json));\
        EXPECT_EQ_INT(CJSON_STRING, get_type(&v));\
        EXPECT_EQ_STRING(expect, get_string(&v), get_string_length(&v));\
        cjson_free(&v);\
    } while(0)

static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
#if 1
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
	TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_null()
{
	cjson_value v;
	v.type = CJSON_TRUE;
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "null"));
	EXPECT_EQ_INT(CJSON_NULL, get_type(&v));
}

static void test_parse_false()
{
	cjson_value v;
	v.type = CJSON_NULL;
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "false"));
	EXPECT_EQ_INT(CJSON_FALSE, get_type(&v));
}

static void test_parse_true()
{
	cjson_value v;
	v.type = CJSON_NULL;
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "true"));
	EXPECT_EQ_INT(CJSON_TRUE, get_type(&v));
}

static void test_parse_not_singular()
{
	cjson_value v;
	v.type = CJSON_NULL;
	EXPECT_EQ_INT(CJSON_PARSE_NOT_SINGULAR, cjson_parse(&v, "true x"));

	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "true "));
	EXPECT_EQ_INT(CJSON_TRUE, get_type(&v));
	
#if 1//below tests are supposed to fail
	/* invalid number */
	TEST_ERROR(CJSON_PARSE_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(CJSON_PARSE_NOT_SINGULAR, "0x0");
	TEST_ERROR(CJSON_PARSE_NOT_SINGULAR, "0x123");
#endif
}

static void test_parse_expect_value()
{
	cjson_value v;
	v.type = CJSON_FALSE;
	EXPECT_EQ_INT(CJSON_PARSE_EXPECT_VALUE, cjson_parse(&v, ""));
	EXPECT_EQ_INT(CJSON_NULL, get_type(&v));

	EXPECT_EQ_INT(CJSON_PARSE_EXPECT_VALUE, cjson_parse(&v, " "));
	EXPECT_EQ_INT(CJSON_NULL, get_type(&v));
}

static void test_access_string()
{
	cjson_value v;
	cjson_init(&v);
	set_string(&v, "", 0);
	EXPECT_EQ_STRING("", get_string(&v), get_string_length(&v));
	set_string(&v, "zhendiniupi", 11);
	EXPECT_EQ_STRING("zhendiniupi", get_string(&v), get_string_length(&v));

	cjson_free(&v);
}

static void test_access_number()
{
	cjson_value v;
	cjson_init(&v);
	set_string(&v, "a", 1);
	set_number_value(&v, 0.0);
	EXPECT_EQ_DOUBLE(0.0, get_number_value(&v));
	set_number_value(&v, 1112.1112);
	EXPECT_EQ_DOUBLE(1112.1112, get_number_value(&v));
	cjson_free(&v);
}

static void test_access_boolean()
{
	cjson_value v;
	cjson_init(&v);
	set_string(&v, "a", 1);
	set_boolean(&v, 0);
	EXPECT_EQ_INT(CJSON_FALSE, get_boolean(&v));
	set_boolean(&v, 1);
	EXPECT_EQ_INT(CJSON_TRUE, get_boolean(&v));
	cjson_free(&v);//remember to free
}

static void test_parse_invalid_value()
{
	cjson_value v;
	v.type = CJSON_FALSE;
	EXPECT_EQ_INT(CJSON_PARSE_INVALID_VALUE, cjson_parse(&v, "nul"));
	EXPECT_EQ_INT(CJSON_PARSE_INVALID_VALUE, cjson_parse(&v, "?"));

	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "[,]");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "[1,]");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "[\"a\", nul]");
	
	//invalid number
#if 1
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(CJSON_PARSE_INVALID_VALUE, "nan");
#endif
}

#define TEST_NUMBER(expect, json)\
	do {\
		cjson_value v; \
		EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, json));\
		EXPECT_EQ_INT(CJSON_NUMBER, get_type(&v));\
		EXPECT_EQ_DOUBLE(expect, get_number_value(&v));\
		}while(0)\

static void test_parse_number_to_big()
{
#if 1
	TEST_ERROR(CJSON_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(CJSON_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif // 0

}

static void test_parse_number()
{
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1e+10, "1e+10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000");//溢出
	//TEST_NUMBER(0.0, "1");
	TEST_NUMBER(1.7976931348623157e308, "1.7976931348623157e308");//max double
	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
	//TEST_NUMBER(4.9406564584124654*(1E−324), "4.9406564584124654E−324");
}

static void test_parse_invalid_string_escape() {
#if 1
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(CJSON_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_parse_invalid_unicode_hex() {
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(CJSON_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array()
{
	cjson_value v;
	cjson_init(&v);
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "[]"));
	EXPECT_EQ_INT(CJSON_ARRAY, v.type);
	EXPECT_EQ_INT(0, v.array_size);
	//

	cjson_init(&v);
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "[1]"));
	EXPECT_EQ_INT(CJSON_ARRAY, v.type);
	EXPECT_EQ_INT(1, v.array_size);
	EXPECT_EQ_INT(CJSON_NUMBER, get_array_element(&v, 0)->type);
	EXPECT_EQ_DOUBLE(1.0, get_array_element(&v, 0)->n);
#if 1

	cjson_init(&v);
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "[  ]"));
	EXPECT_EQ_INT(CJSON_ARRAY, get_type(&v));
	//EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
	EXPECT_EQ_INT(0, v.array_size);

	cjson_init(&v);
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "[ 3   ,2   ]"));
	EXPECT_EQ_INT(CJSON_ARRAY, v.type);
	EXPECT_EQ_INT(2, v.array_size);
	EXPECT_EQ_INT(CJSON_NUMBER, get_array_element(&v, 0)->type);
	EXPECT_EQ_DOUBLE(3.0, get_array_element(&v, 0)->n);
	EXPECT_EQ_DOUBLE(2.0, get_array_element(&v, 1)->n);

	cjson_init(&v);
	EXPECT_EQ_INT(CJSON_PARSE_OK, cjson_parse(&v, "[1, \"1\", [1,2, 3 ,4,[]] ]"));
	EXPECT_EQ_INT(CJSON_ARRAY, v.type);
	EXPECT_EQ_INT(3, v.array_size);
	EXPECT_EQ_INT(CJSON_NUMBER, get_array_element(&v, 0)->type);
	EXPECT_EQ_DOUBLE(1.0, get_array_element(&v, 0)->n);
	EXPECT_EQ_INT(CJSON_ARRAY, get_array_element(&v, 2)->type);
	EXPECT_EQ_INT(5, get_array_element(&v, 2)->array_size);
	//EXPECT_EQ_DOUBLE(3.0, get_array_element(&v, 2)->n);
	EXPECT_EQ_INT(CJSON_STRING, get_array_element(&v, 1)->type);
	("1", get_array_element(&v, 1)->s, 1);
#endif // 
	cjson_free(&v);
}

static void test_parse_miss_comma_or_square_bracket() {
#if 1
	TEST_ERROR(CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(CJSON_PARSE_ARRAY_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
#endif
}
//milo's tutorial include it in INVALID_VALUE, that's better than my solution, cause we have case like [,]
//though we can do specified thing to it, but i prefer to follow milo's, cause i'm lazy...
/*
static void test_parse_array_extra_comma() {
	TEST_ERROR(CJSON_PARSE_ARRAY_EXTRA_COMMA, "[1,2,]");
	TEST_ERROR(CJSON_PARSE_ARRAY_EXTRA_COMMA, "[1,]");
	TEST_ERROR(CJSON_PARSE_ARRAY_EXTRA_COMMA, "[,]");
}
*/

void test_parse()
{
	test_parse_null();
	test_parse_false();
	test_parse_true();
	test_parse_string();
	test_access_boolean();
	test_access_number();
	test_access_string();
	test_parse_not_singular();
	test_parse_expect_value();
	test_parse_number();
	test_parse_number_to_big();
	test_parse_invalid_value();
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();
	test_parse_invalid_unicode_hex();
	test_parse_invalid_unicode_surrogate();
	/**/
	test_parse_array();
	test_parse_miss_comma_or_square_bracket();
	//test_parse_array_extra_comma();

	if (main_ret == 0) {
		printf("ok no problem!\n");
	}
}

int main()
{
	malloc(sizeof(char));
	//_CrtDumpMemoryLeaks();
	//printf("%d %d", sizeof(char), sizeof('\n'));
	test_parse();

	getchar();

	/*
	char ch;
	printf("%d %d %d", sizeof(ch), sizeof(char), sizeof('a'));
	*/
	
	/*
	int i;
	sscanf_s("0x10", "%3x", &i);
	printf("%d ", i);
	*/
	
	/*
	int c = 65;
	char* p = malloc(10);
 	sprintf(p, "%c", c);//"A..." just the same as -> *p = c; except the the extra '\0'
	//*(char*)p = c;
	free(p);
	*/
	
	//getchar();

	return main_ret;
}