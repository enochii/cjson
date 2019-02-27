# cjson

明目张胆抄轮子改变量名...[json-tutorial](https://github.com/miloyip/json-tutorial)

**1.`sizeof('a') != sizeof(`char`)`**

在写string部分处理转义字符的时候发现，这样:

```c++
\\调用方已经把`\`读了
#define HANDLE_ESCAPE(c, p) \
do{\
        char ch = *p;\
        switch(ch){\
            case 'n':PUTC(c, '\n');break;\
            ...
        }\
}while(0)
```

是不行的，因为PUTC的定义像这样:

```c++
#define PUTC(c, ch) *(char*)(cjson_context_push((c), sizeof(ch))) = ch
```

又或者修改PUTC的定义 ..., sizeof(char) ...

[栈溢出](https://stackoverflow.com/questions/2172943/size-of-character-a-in-c-c)
>In C, the type of a character constant like 'a' is actually an int, with size of 4 (or some other implementation-dependent value). In C++, the type is char, with size of 1. This is one of many small differences between the two languages.

就是你把那个存字符的stack缓冲区空了几个位..感觉我好像听谁说过-0-

**2.内存泄露先留个坑...**