#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "json.h"

#define CNT_MASK (~(JSON_ROOT | JSON_OBJECT | JSON_ARRAY | JSON_VALUE))
#define ALIGN_MASK (sizeof(void *) - 1)
#define	NODE_SIZE (256)
#define JSON_SPACE "\r\n "

#define DBG_PRINT(...)	printf(__VA_ARGS__)
//#define DBG_PRINT(...)	      ((void)0)

#define	EXPECT(str, ch, errmsg)	\
	if (*str != ch) {\
		DBG_PRINT(errmsg);\
		return NULL;\
	} else {\
		++str;\
	}

#define my_malloc(a) malloc(a)
#define	my_realloc(ptr, a) realloc(ptr, a)
#define my_free(a) free(a)

struct node {
	int cap;
	struct node *next;
	//data append tail;
};

struct pool {
	int offset;
	struct node *nodes;
};


struct json {
	union {
		char *name;
		struct pool *pool;
	} u;
	struct json_data data;
};

static struct pool *
pool_create()
{
	struct pool *p;
	p = (struct pool *)my_malloc(sizeof(*p));
	memset(p, 0, sizeof(*p));
	return p;
}

static void
pool_free(struct pool *p)
{
	struct node *n = p->nodes;
	while (n) {
		struct node *t = n;
		n = n->next;
		my_free(t);
	}
	my_free(p);
}

static void *
pool_alloc(struct pool *p, size_t sz)
{
	void *ret;
	struct node *i;
	sz = (sz + ALIGN_MASK) & (~ALIGN_MASK);
	i = p->nodes;
	if (!i || i->cap - p->offset < sz) {
		size_t need;
		struct node *n;
		if (sz > NODE_SIZE)
			need = (sz + ALIGN_MASK) & (~ALIGN_MASK);
		else
			need = NODE_SIZE;
		need += sizeof(struct node);
		n = my_malloc(need);
		n->cap = need;
		p->offset = 0;
		n->next = p->nodes;
		p->nodes = n;
		i = n;
	}
	assert(i->cap - p->offset == sz);
	ret = (void *)(i + 1) + p->offset;
	p->offset += sz;
	return ret;
}

static char *
pool_dupstr(struct pool *p, const char *start, const char *end)
{
	size_t sz = end - start;
	char *str = (char *)pool_alloc(p, sz + 1);
	memcpy(str, start, sz);
	str[sz] = '\0';
	return str;
}

static struct json *
nodenew(struct pool *p, int type)
{
	struct json *j;
	j = (struct json *)pool_alloc(p, sizeof(struct json));
	memset(j, 0, sizeof(*j));
	j->data.type = type;
	return j;
}

static struct json *
nodeexpand(struct json *j, int type, int n)
{
	j = (struct json *)my_realloc(j, (n + 1) * sizeof(struct json));
	memset(&j[n], 0, sizeof(struct json));
	j[n].data.type = type;
	return j;
}

//TODO:check grammer
static int
check_name(const char *str, size_t sz)
{
	return 1;
}

static int
check_string(const char *str, size_t sz)
{
	return 1;
}



struct json *
json_create()
{
	struct json *j;
	struct pool *p;
	p = pool_create();
	j = nodenew(p, JSON_ROOT | JSON_OBJECT);
	j->u.pool = p;
	return j;
}

void
json_free(struct json *j)
{
	assert(j->type == JSON_ROOT);
	pool_free(j->u.pool);
}

int
json_load(struct json *J, const char *path)
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

static inline const char *
skip_to(const char *str, char ch)
{
        while (*str != ch && *str != 0)
                str++;
        if (*str)
                str++;
        return str;
}

static inline const char *
skip_space(const char *str)
{
	const char *skip;
	for (; *str; str++) {
		for (skip = JSON_SPACE; *skip; skip++) {
			if (*str == *skip)
				break;
		}
		if (*skip == 0)
			break;
	}
	return str;
}

static const char *
load(struct pool *p, struct json_data *jd, const char *str)
{
	int nr = 0;
	const char *start;
	if ((jd->type & JSON_OBJECT) == JSON_OBJECT) {
		struct json *arr = NULL;
		str = skip_space(str);
		EXPECT(str, '{', "json object should open with '{'\n")
		str = skip_space(str);
		while (*str != '}' && *str != '\0') {
			EXPECT(str, '"', "json value name should open with '\"' ");
			arr = nodeexpand(arr, JSON_VALUE, nr);
			start = str;
			str = skip_to(str, '"');
			arr[nr].u.name = pool_dupstr(p, start, str);
			DBG_PRINT("occurs value:%s\n", arr[nr].u.name);
			if (!check_name(arr[nr].u.name, str - start))
				return NULL;
			EXPECT(str, ':', "json value should sperate by ':'\n");
			str = load(p, &arr[nr].data, str);
			if (str == NULL) {
				DBG_PRINT("free it\n");
				my_free(arr);
				return NULL;
			}
			++nr;
			str = skip_space(str);
			if (*str == ',')
				str++;
		}
		//ok, all array element has parsed
		if (*str == '}') {
			str++;
		} else {
			my_free(arr);
			return NULL;
		}
		jd->type |= nr;
		nr = nr * sizeof(struct json);
		jd->v.s = pool_alloc(p, nr);
		memcpy(jd->v.s, arr, nr);
		my_free(arr);
		return str;
	} else if (jd->type & JSON_VALUE) {
		//judge if array or real value
		str = skip_space(str);
		if (*str == '\0')
			return NULL;
		if (*str == '"') {	//real value(char)
			start = ++str;
			jd->type = JSON_STRING;
			str = skip_to(str, '"');
			if (*str == '\0')
				return NULL;
			jd->v.c = pool_dupstr(p, start, str);
			if (!check_string(jd->v.c, str - start))
				return NULL;
			str = skip_space(str);
			if (*str == ',')
				str++;
			return str;
		} else if (isdigit(*str)) {//TODO:process more types
			char *end;
			start = str;
			jd->type = JSON_NUMBER;
			jd->v.n = (int)strtoul(str, &end, 0);
			str = end;
			if (start == str)
				return NULL;
			return str;
		} else if (strncmp(str, "true", sizeof("true") - 1) == 0) {
			jd->type = JSON_BOOLEAN;
			jd->v.b = 1;
			return str + sizeof("true") - 1;
		} else if (strncmp(str, "false", sizeof("false") - 1) == 0) {
			jd->type = JSON_BOOLEAN;
			jd->v.b = 0;
			return str + sizeof("false") - 1;
		} else if (*str == '[') {
			jd->type = JSON_ARRAY;
			return load(p, jd, str);
		} else if (*str == '{') {
			jd->type = JSON_OBJECT;
			return load(p, jd, str);
		} else {
			DBG_PRINT("saddly value:%s\n", str);
			assert(!"saddly!");
			return NULL;
		}
	} else if ((jd->type & JSON_ARRAY) == JSON_ARRAY) {
			DBG_PRINT("occurs array\n");
			struct json *arr = NULL;
			assert(*str == '[');
			EXPECT(str, '[', "json array should open with '['\n");
			while (*str != ']' && *str != '\0') {
				arr = nodeexpand(arr, JSON_OBJECT, nr);
				str = load(p, &arr[nr].data, str);
				if (str == NULL) {
					my_free(arr);
					return NULL;
				}
				if (*str == ',')
					str++;
				nr++;
			}
			str = skip_space(str);
			EXPECT(str, ']', "json array should close with ']'\n");
			if (*str == '\0') {
				my_free(arr);
				return NULL;
			}
			jd->type |= nr;
			nr = nr * sizeof(struct json);
			jd->v.s = (struct json *)pool_alloc(p, nr);
			memcpy(jd->v.s, arr, nr);
			my_free(arr);
			return str;
	} else {
		DBG_PRINT("str:%s, J->data.type:%d\n", str, jd->type);
		assert(!"never come here");
		return NULL;
	}
}

int
json_loadstring(struct json *j, const char *str)
{
	const char *res;
	struct pool *p;
	assert(j->data.type & JSON_ROOT);
	assert(j->data.type & JSON_OBJECT);
	assert(j->u.pool);
	assert(j->data.v.s);
	p = j->u.pool;
	res = load(p, &j->data, str);
	if (res == NULL)
		return -1;
	return 0;
}

struct json *
json_getbyname(struct json *J, const char *name)
{
	int i;
	struct json *data;
	assert(J);
	assert(name);

	data = J->data.v.s;
	if (data == NULL)
		return NULL;

	for (i = 0; i < (J->data.type & CNT_MASK); i++, data++) {
		if (data->u.name && strcmp(data->u.name, name) == 0)
			return data;
	}

	return NULL;
}

struct json *
json_getbyindex(struct json *J, int index)
{
	assert(J);
	assert(index < (J->data.type & CNT_MASK));

	return &J->data.v.s[index];
}

const char *
json_getname(struct json *J)
{
	assert(J);

	return J->u.name;
}

const struct json_data *
json_getdata(struct json *J)
{
	assert(J);
	return &J->data;
}

int
json_getdatacnt(struct json *J)
{
	assert(J);
	return (J->data.type & CNT_MASK);
}

int
json_gettype(struct json *J)
{
	assert(J);

	return (J->data.type & (~CNT_MASK));
}


void
json_dump(struct json *J)
{
	int i;
	if (J == NULL)
		return ;
	if (J->data.type & JSON_OBJECT) {
		for (i = 0; i < (J->data.type & CNT_MASK); i++) {
			if (J->u.name && !(J->data.type & JSON_ROOT))
				printf("object name:%s ", J->u.name);
			json_dump(&J->data.v.s[i]);
		}
	} else if (J->data.type & JSON_VALUE) {
		switch (J->data.type) {
		case JSON_BOOLEAN:
			printf("name->%s:value->boolean, %s\n", J->u.name, J->data.v.b ? "true" : "false");
			break;
		case JSON_NUMBER:
			printf("name->%s:value->%d\n", J->u.name, J->data.v.n);
			break;
		case JSON_STRING:
			printf("name->%s:value->%s\n", J->u.name, J->data.v.c);
			break;
		default:
			printf("Unexpected Json type:0x%x\n", J->data.type);
			assert(0);
		}
	} else if (J->data.type & JSON_ARRAY) {
		for (i = 0; i < (J->data.type & CNT_MASK); i++)
			json_dump(&J->data.v.s[i]);
	}
}

