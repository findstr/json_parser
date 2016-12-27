/**
=========================================================================
 Author: findstr
 Email: findstr@sina.com
 File Name: /home/findstr/code/json.h
 Description: (C)  2015-01  findstr

 Edit History:
   2015-01-25    File created.
=========================================================================
**/

struct json;

#define JSON_ROOT       (1 << 31)
#define JSON_OBJECT     (1 << 30)
#define JSON_ARRAY      (1 << 29)
#define JSON_VALUE      (0xF << 25)

#define JSON_STRING     (1 << 25)
#define JSON_NUMBER     (2 << 25)
#define JSON_DOUBLE     (3 << 25)
#define JSON_BOOLEAN    (4 << 25)

struct json_data {
        int type;
        union {
                struct json     *s;
                char            *c;
                int             n;
                int             b;
        } v;
};


struct json *json_create();
void json_free(struct json *J);

int json_load(struct json *J, const char *path);
int json_loadstring(struct json *J, const char *sz);

struct json *json_getbyname(struct json *J, const char *name);
struct json *json_getbyindex(struct json *J, int index);

const char *json_getname(struct json *J);
const struct json_data *json_getdata(struct json *J);

int json_getdatacnt(struct json *J);
int json_gettype(struct json *J);

//for debug
void json_dump(struct json *J);
