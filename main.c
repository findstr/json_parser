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
			"}"
		"]"
	"},"
	"\"error_code\":0"
        "}";

        struct json *J = json_create();
        struct json *a;
 
        json_loadstring(J, json);
       
        printf("object:%lx, type:%x, cnt:%x\n", (uintptr_t)J, json_gettype(J), json_getchildcnt(J));

        //g is root
        a = json_getbyindex(J, 0);
        
        printf("object1:%lx, type:%x, cnt:%x\n", (uintptr_t)a, json_gettype(a), json_getchildcnt(a));
        
        a = json_getbyindex(a, 0);
        
        printf("name:%s, value:%s\n", json_getname(a), json_getdata(a)->v.c);

        printf("------------dump--------------------\n");


        json_dump(J);

        json_free(J);

        return 0;
}
