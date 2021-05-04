#include <stdio.h>
#include <stdlib.h>
#include <evhttp.h>
#include <event.h>
#include <string.h>
#include "event2/http.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/bufferevent.h"
#include "event2/bufferevent_compat.h"
#include "event2/http_struct.h"
#include "event2/http_compat.h"
#include "event2/util.h"
#include "event2/listener.h"
#include "cjson/cJSON.h"

#define BUF_MAX 1024 * 16

//解析post请求数据
void get_post_message(char *buf, struct evhttp_request *req)
{
    size_t post_size = 0;

    post_size = evbuffer_get_length(req->input_buffer); //获取数据长度
    printf("====line:%d,post len:%d\n", __LINE__, post_size);
    if (post_size <= 0)
    {
        printf("====line:%d,post msg is empty!\n", __LINE__);
        return;
    }
    else
    {
        size_t copy_len = post_size > BUF_MAX ? BUF_MAX : post_size;
        printf("====line:%d,post len:%d, copy_len:%d\n", __LINE__, post_size, copy_len);
        memcpy(buf, evbuffer_pullup(req->input_buffer, -1), copy_len);
        buf[post_size] = '\0';
        printf("====line:%d,post msg:%s\n", __LINE__, buf);
    }
}

//解析http头，主要用于get请求时解析uri和请求参数
char *find_http_header(struct evhttp_request *req, struct evkeyvalq *params, const char *query_char)
{
    if (req == NULL || params == NULL || query_char == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "input params is null.");
        return NULL;
    }

    struct evhttp_uri *decoded = NULL;
    char *query = NULL;
    char *query_result = NULL;
    const char *path;
    const char *uri = evhttp_request_get_uri(req); //获取请求uri

    if (uri == NULL)
    {
        printf("====line:%d,evhttp_request_get_uri return null\n", __LINE__);
        return NULL;
    }
    else
    {
        printf("====line:%d,Got a GET request for <%s>\n", __LINE__, uri);
    }

    //解码uri
    decoded = evhttp_uri_parse(uri);
    if (!decoded)
    {
        printf("====line:%d,It's not a good URI. Sending BADREQUEST\n", __LINE__);
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }

    //获取uri中的path部分
    path = evhttp_uri_get_path(decoded);
    if (path == NULL)
    {
        path = "/";
    }
    else
    {
        printf("====line:%d,path is:%s\n", __LINE__, path);
    }

    //获取uri中的参数部分
    query = (char *)evhttp_uri_get_query(decoded);
    if (query == NULL)
    {
        printf("====line:%d,evhttp_uri_get_query return null\n", __LINE__);
        return NULL;
    }

    //查询指定参数的值
    evhttp_parse_query_str(query, params);
    query_result = (char *)evhttp_find_header(params, query_char);

    return query_result;
}

//处理get请求
void http_handler_testget_msg(struct evhttp_request *req, void *arg)
{
    if (req == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }

    char *sign = NULL;
    char *data = NULL;
    struct evkeyvalq sign_params = {0};
    sign = find_http_header(req, &sign_params, "sign"); //获取get请求uri中的sign参数
    if (sign == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "request uri no param sign.");
    }
    else
    {
        printf("====line:%d,get request param: sign=[%s]\n", __LINE__, sign);
    }

    data = find_http_header(req, &sign_params, "data"); //获取get请求uri中的data参数
    if (data == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "request uri no param data.");
    }
    else
    {
        printf("====line:%d,get request param: data=[%s]\n", __LINE__, data);
    }
    printf("\n");

    //回响应
    struct evbuffer *retbuff = NULL;
    retbuff = evbuffer_new();
    if (retbuff == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    evbuffer_add_printf(retbuff, "Receive get request,Thamks for the request!");
    evhttp_send_reply(req, HTTP_OK, "Client", retbuff);
    evbuffer_free(retbuff);
}

//处理post请求
void http_handler_testpost_msg(struct evhttp_request *req, void *arg)
{
    if (req == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "input param req is null.");
        return;
    }

    char buf[BUF_MAX] = {0};
    get_post_message(buf, req); //获取请求数据，一般是json格式的数据
    if (buf == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "get_post_message return null.");
        return;
    }
    else
    {
        //可以使用json库解析需要的数据
        printf("====line:%d,request data:%s", __LINE__, buf);
        cJSON *cjson_test = NULL;
        cJSON *cjson_name = NULL;
        cJSON *cjson_age = NULL;
        cJSON *cjson_weight = NULL;
        cJSON *cjson_address = NULL;
        cJSON *cjson_address_country = NULL;
        cJSON *cjson_address_zipcode = NULL;
        cJSON *cjson_skill = NULL;
        cJSON *cjson_student = NULL;
        int skill_array_size = 0, i = 0;
        cJSON *cjson_skill_item = NULL;

        /* 解析整段JSO数据 */
        cjson_test = cJSON_Parse(buf);
        if (cjson_test == NULL)
        {
            printf("parse fail.\n");
            return;
        }

        /* 依次根据名称提取JSON数据（键值对） */
        cjson_name = cJSON_GetObjectItem(cjson_test, "name");
        cjson_age = cJSON_GetObjectItem(cjson_test, "age");
        cjson_weight = cJSON_GetObjectItem(cjson_test, "weight");

        printf("name: %s\n", cjson_name->valuestring);
        printf("age:%d\n", cjson_age->valueint);
        printf("weight:%.1f\n", cjson_weight->valuedouble);

        /* 解析嵌套json数据 */
        cjson_address = cJSON_GetObjectItem(cjson_test, "address");
        cjson_address_country = cJSON_GetObjectItem(cjson_address, "country");
        cjson_address_zipcode = cJSON_GetObjectItem(cjson_address, "zip-code");
        printf("address-country:%s\naddress-zipcode:%d\n", cjson_address_country->valuestring, cjson_address_zipcode->valueint);

        /* 解析数组 */
        cjson_skill = cJSON_GetObjectItem(cjson_test, "skill");
        skill_array_size = cJSON_GetArraySize(cjson_skill);
        printf("skill:[");
        for (i = 0; i < skill_array_size; i++)
        {
            cjson_skill_item = cJSON_GetArrayItem(cjson_skill, i);
            printf("%s,", cjson_skill_item->valuestring);
        }
        printf("\b]\n");

        /* 解析布尔型数据 */
        cjson_student = cJSON_GetObjectItem(cjson_test, "student");
        if (cjson_student->valueint == 0)
        {
            printf("student: false\n");
        }
        else
        {
            printf("student:error\n");
        }
    }

    //回响应

    cJSON *root = NULL;
    cJSON *fmt = NULL;
    char *jsondata = NULL;
    size_t len = 0;
    char *out = NULL;

    /* Our "Video" datatype: */
    root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
    // cJSON_AddItemToObject(root, "format", fmt = cJSON_CreateObject());
    cJSON_AddStringToObject(root, "statu_code", "AAAAAA");
    cJSON_AddStringToObject(root, "error_desc", "succeed");

    /* formatted print */
    out = cJSON_Print(root);
    len = strlen(out) + 5;
    jsondata = (char *)malloc(len);
    if (jsondata == NULL)
    {
        printf("Failed to allocate memory.\n");
        exit(1);
    }
    /* Print to buffer */
    if (!cJSON_PrintPreallocated(root, jsondata, (int)len, 1))
    {
        printf("cJSON_PrintPreallocated failed!\n");
        if (strcmp(out, jsondata) != 0)
        {
            printf("cJSON_PrintPreallocated not the same as cJSON_Print!\n");
            printf("cJSON_Print result:\n%s\n", out);
            printf("cJSON_PrintPreallocated result:\n%s\n", buf);
        }
        free(out);
        free(jsondata);
        return;
    }
    struct evbuffer *retbuff = NULL;
    retbuff = evbuffer_new();
    if (retbuff == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "retbuff is null.");
        return;
    }
    evbuffer_add_printf(retbuff, jsondata);
    evhttp_send_reply(req, HTTP_OK, "Client", retbuff);
    free(jsondata);
    free(out);
    cJSON_Delete(root);
    evbuffer_free(retbuff);
}

int main()
{
    struct evhttp *http_server = NULL;
    short http_port = 8081;
    char *http_addr = "0.0.0.0";

    //初始化
    event_init();
    //启动http服务端
    http_server = evhttp_start(http_addr, http_port);
    if (http_server == NULL)
    {
        printf("====line:%d,%s\n", __LINE__, "http server start failed.");
        return -1;
    }

    //设置请求超时时间(s)
    evhttp_set_timeout(http_server, 5);
    //设置事件处理函数，evhttp_set_cb针对每一个事件(请求)注册一个处理函数，
    //区别于evhttp_set_gencb函数，是对所有请求设置一个统一的处理函数
    evhttp_set_cb(http_server, "/me/testpost", http_handler_testpost_msg, NULL);
    evhttp_set_cb(http_server, "/me/testget", http_handler_testget_msg, NULL);

    //循环监听
    event_dispatch();
    //实际上不会释放，代码不会运行到这一步
    evhttp_free(http_server);

    return 0;
}
