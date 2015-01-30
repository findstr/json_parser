/**
=========================================================================
 Author: findstr
 Email: findstr@sina.com
 File Name: json.c
 Description: (C)  2015-01  findstr
   
 Edit History: 
   2015-01-25    File created.
=========================================================================
**/
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "json.h"

#define CNT_MASK        (~(JSON_ROOT | JSON_OBJECT | JSON_ARRAY | JSON_VALUE))
#define JSON_SPACE      "\r\n "

//#define DBG_PRINT(...)        printf(__VA_ARGS__)
#define DBG_PRINT(...)        ((void)0)

#define my_malloc(a)    malloc(a)
#define my_free(a)      free(a)

struct json {
        char *name;
        struct json_data child;
};

static struct json *
_new_node(int type)
{
        struct json *J;

        J = (struct json *)my_malloc(sizeof(struct json));

        memset(J, 0, sizeof(*J));
        
        J->child.type = type;

        return J;

}

static struct json *
_expand_one_node(struct json *J, int type, int n)
{
        J = (struct json *)realloc(J, (n + 1) * sizeof(struct json));

        memset(&J[n], 0, sizeof(struct json));

        J[n].child.type = type;

        return J;
}

struct json *json_create()
{
        return _new_node(JSON_ROOT);
}

void json_free(struct json *J)
{
        int i;
        if (J == NULL)
                return ;

        if (J->child.type & JSON_ROOT) {
                json_free(J->child.v.s);
                assert(J->name);
                my_free(J->name);
                my_free(J->child.v.s);
                my_free(J);
        } else if (J->child.type & JSON_OBJECT) {
                for (i = 0; i < (J->child.type & CNT_MASK); i++)
                        json_free(&J->child.v.s[i]);

                my_free(J->child.v.s);
        } else if (J->child.type & JSON_ARRAY) {
                for (i = 0; i < (J->child.type & CNT_MASK); i++)
                        json_free(&J->child.v.s[i]);

                my_free(J->child.v.s);
        }
}

int json_load(struct json *J, const char *path)
{
        int err;
        FILE *fp;
        char *buff;
        struct stat   st;
        
        err = stat(path, &st);
        if (err < 0)
                return -1;

        fp = fopen(path, "rb");
        if (fp == NULL)
                return -1;

        buff = (char *)my_malloc(st.st_size + 1);
        assert(buff);
        
        if (fread(buff, st.st_size, 1, fp) != 1) {
                free(buff);
                fclose(fp);
                return -1;
        }

        buff[st.st_size] = 0;
        
        err = json_loadstring(J, buff);

        fclose(fp);
        free(buff);

        return err;
}

static __inline char *
_skip_to(char *sz, char ch)
{
        while (*sz != ch && *sz != 0)
                sz++;
        
        assert(*sz);

        sz++;
        
        return sz;
}

static char *
_skip_space(char *sz)
{
        const char *skip;

        for (; *sz; sz++) {
                for (skip = JSON_SPACE; *skip; skip++) {
                        if (*sz == *skip)
                                break;
                }

                if (*skip == 0)
                        break;
        }

        return sz;
}

static char *
_load_recursive(struct json *J, char *sz)
{
        int nr;
        
        nr = 0;
        if ((J->child.type & JSON_OBJECT) == JSON_OBJECT) {
                struct json             *arr = NULL;
                sz = _skip_space(sz);
                
                assert(*sz == '{');

                sz = _skip_to(sz, '{');
                sz = _skip_space(sz);

                while (*sz != '}' && *sz != '\0') {
                        if (*sz == '"') {
                                arr = _expand_one_node(arr, JSON_VALUE, nr);
                                sz = _load_recursive(&arr[nr], sz);
                        } else if (*sz == '{') {
                                arr = _expand_one_node(arr, JSON_OBJECT, nr);
                                sz = _load_recursive(&arr[nr], sz);
                        } else {
                                DBG_PRINT("object, %s", sz);
                                assert(!"OMG");
                        }

                        nr++;
                        sz = _skip_space(sz);

                        if (*sz == ',')
                                sz++;
                }

                assert(*sz == '}');
                if (*sz == '}')
                        sz++;

                J->child.v.s = arr;
                J->child.type |= nr;
                return sz;
        } else if (J->child.type & JSON_VALUE) {
                        //name
                        sz = _skip_to(sz, '"');
                        J->name = sz;
                        sz= _skip_to(sz, '"');
                        sz[-1] = 0;
                        //judge if array or real value
                        sz = _skip_space(sz);
                        sz = _skip_space(sz+ 1);
                        
                        DBG_PRINT("occurs value:%s\n", J->name);
                        
                        if (*sz == '"') {       //real value(char)
                                J->child.type = JSON_STRING;
                                sz = _skip_to(sz, '"');
                                J->child.v.c = sz;
                                sz = _skip_to(sz, '"');
                                sz[-1] = 0;
                                sz = _skip_space(sz);
                                if (*sz == ',')
                                        sz++;
                                
                                return sz;
                        } else if (isdigit(*sz)) {
                                J->child.type = JSON_NUMBER;
                                J->child.v.n = (int)strtoul(sz, &sz, 0);
                                return sz;
                        } else if (strncmp(sz, "true", sizeof("true")) == 0) {
                                J->child.type = JSON_BOOLEAN;
                                J->child.v.b = 1;
                                return sz + sizeof("true");
                        } else if (strncmp(sz, "false", sizeof("false")) == 0) {
                                J->child.type = JSON_BOOLEAN;
                                J->child.v.b = 1;
                                return sz + sizeof("false");
                        } else if (*sz== '[') {
                                J->child.type = JSON_ARRAY;
                                return _load_recursive(J, sz);
                        } else if (*sz == '{') {
                                J->child.type = JSON_OBJECT;
                                return _load_recursive(J, sz);
                        } else {
                                DBG_PRINT("saddly value:%s\n", sz);
                                assert(!"saddly!");
                        }

        } else if ((J->child.type & JSON_ARRAY) == JSON_ARRAY) {
                        DBG_PRINT("occurs array\n");
                        
                        struct json *arr = NULL;

                        assert(*sz == '[');
                        sz = _skip_to(sz, '[');

                        while (*sz != ']' && *sz != 0) {
                                arr = _expand_one_node(arr, JSON_OBJECT, nr);
                                sz = _load_recursive(&arr[nr], sz);
                                if (*sz == ',')
                                        sz++;
                                nr++;
                        }

                        J->child.v.s = arr;
                        J->child.type |= nr;
                        
                        DBG_PRINT("JSON_ARRAY:%s", sz);
                        assert(*sz == ']');

                        return sz + 1;
        } else {
                DBG_PRINT("sz:%s, J->child.type:%d\n", sz, J->child.type);
                assert(!"never come here");
        }

}

int json_loadstring(struct json *J, const char *sz)
{
        char *bk;
        char *res;

        assert(J->child.type & JSON_ROOT);
        
        bk = strdup(sz);
        assert(bk);
        J->child.type |= 1;
        J->name = bk;
        J->child.v.s = _new_node(JSON_OBJECT);
        
        res = _load_recursive(J->child.v.s, bk); 
        assert(*res == 0);

        if (res == NULL)
                return -1;

        return 0;
}

struct json *json_getbyname(struct json *J, const char *name)
{
        int i;
        struct json *child;
        assert(J);
        assert(name);

        child = J->child.v.s;
        if (child == NULL)
                return NULL;

        for (i = 0; i < (J->child.type & CNT_MASK); i++, child++) {
                if (child->name && strcmp(child->name, name) == 0)
                        return child;
        }

        return NULL;
}

struct json *json_getbyindex(struct json *J, int index)
{
        assert(J);
        assert(index < (J->child.type & CNT_MASK));

        return &J->child.v.s[index];
}

const char *json_getname(struct json *J)
{
        assert(J);

        return J->name;
}

const struct json_data *json_getdata(struct json *J)
{
        assert(J);
        return &J->child;
}

int json_getchildcnt(struct json *J)
{
        assert(J);

        return (J->child.type & CNT_MASK);
}

int json_gettype(struct json *J)
{
        assert(J);

        return (J->child.type & (~CNT_MASK));
}


void json_dump(struct json *J)
{
        int i;
        if (J == NULL)
                return ;

        if (J->child.type & JSON_ROOT) {
                json_dump(J->child.v.s);
        } if (J->child.type & JSON_OBJECT) {
                for (i = 0; i < (J->child.type & CNT_MASK); i++) {
                        if (J->name)
                                printf("object name:%s ", J->name);
                        json_dump(&J->child.v.s[i]);
                }
        } else if (J->child.type & JSON_VALUE) {
                switch (J->child.type) {
                case JSON_BOOLEAN:
                        printf("name->%s:value->%s\n", J->name, J->child.v.b ? "true" : "false");
                        break;
                case JSON_NUMBER:
                        printf("name->%s:value->%d\n", J->name, J->child.v.n);
                        break;
                case JSON_STRING:
                        printf("name->%s:value->%d\n", J->name, J->child.v.n);
                        break;
                default:
                        printf("Unexpected Json type:0x%x\n", J->child.type);
                        assert(0);
                }
        } else if (J->child.type & JSON_ARRAY) {
                for (i = 0; i < (J->child.type & CNT_MASK); i++)
                        json_dump(&J->child.v.s[i]);
        }
}

