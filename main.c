/**
=========================================================================
 Author: findstr
 Email: findstr@sina.com
 File Name: main.c
 Description: (C)  2015-01  findstr

 Edit History:
   2015-01-25    File created.
=========================================================================
**/
#include <stdint.h>
#include <stdio.h>
#include "json.h"

int main()
{
        const char *json = " {"
	"\"resultcode\":\"200\","
	"\"reason\":\"Return Successd!\","
	"\"result:\":{"
		"\"data\":"
		        "["
                                "{"
				"\"MCC\":\"0\","
				"\"MNC\":\"0\","
				"\"LAC\":\"6145\","
				"\"CELL\":\"24961\","
				"\"LNG\":\"121.389263\","
				"\"LAT\":\"31.08041\","
				"\"O_LNG\":\"121.39390245226\","
				"\"O_LAT\":\"31.078499620226\","
				"\"PRECISION\":\"1032\","
				"\"ADDRESS\":\"上海市闵行区申南路\""
                                "\"Boolean\":true"
			"}"
		"]"
	"},"
	"\"error_code\":0"
        "}";

        int err;
        struct json *J = json_create();
        struct json *a;
        err = json_loadstring(J, json);
        printf("json_loadstring:%d\n", err);
        if (err < 0)
		goto end;
        printf("object:%p, type:%x, cnt:%d\n", J, json_gettype(J), json_getdatacnt(J));
        //g is root
        a = json_getbyindex(J, 2);
        printf("object1:%p, name:%s, type:%x, cnt:%d\n", a, json_getname(a),
			json_gettype(a), json_getdatacnt(a));
        a = json_getbyindex(a, 0);
        printf("name:%s, type:%x\n", json_getname(a), json_getdata(a)->type);
        printf("------------dump--------------------\n");
        json_dump(J);
end:
        json_free(J);
        return 0;
}
