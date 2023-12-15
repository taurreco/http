#include "http.c"
#include "unity.h"

/*********************************************************************
 *                                                                   *
 *                          unity helpers                            *
 *                                                                   *
 *********************************************************************/

void 
setUp() 
{
    /* empty */
}

void 
tearDown() 
{
    /* empty */
}

/*********************************************************************
 *                                                                   *
 *                           basic tests                             *
 *                                                                   *
 *********************************************************************/

/*************
 * basic_get *
 *************/

void
basic_get()
{
    struct request req;
    int status;

    request_init(&req);
    status = parse_request(&req, "GET / HTTP/1.0\r\n\r\n");

    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_INT(GET, req.method);
    TEST_ASSERT_EQUAL_STRING("/", req.uri);
}

/***************
 * basic_split *
 ***************/

void
basic_split()
{
    char *left, *right;
    char* src = "hello world!";

    split(src, 5, &left, &right);

    TEST_ASSERT_EQUAL_STRING("hello", left);
    TEST_ASSERT_EQUAL_STRING(" world!", right);

    free(left);
    free(right);
}

/*****************
 * split_on_html *
 *****************/

void
split_on_html()
{
    char *cur, *left, *right;
    char* src = "<hello></world!>";

    cur = src;
    next_angle_bracket(&cur);

    split(src, cur - src + 1, &left, &right);

    TEST_ASSERT_EQUAL_STRING("<hello>", left);
    TEST_ASSERT_EQUAL_STRING("</world!>", right);

    free(left);
    free(right);
}

/********************
 * put_with_headers *
 ********************/

void
put_with_headers()
{
    struct request req;
    int status;
    char* raw = "POST /login.html HTTP/1.0\r\n"
                "Host: localhost:8080\r\n"
                "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:120.0)"
                "Gecko/20100101 Firefox/120.0\r\n"
                "Accept: text/html,application/xhtml+xml,"
                "application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
                "Accept-Language: en-US,en;q=0.5\r\n"
                "Accept-Encoding: gzip, deflate, br\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: 30\r\n"
                "Origin: http://localhost:8080\r\n"
                "Connection: keep-alive\r\n"
                "Referer: http://localhost:8080/\r\n"
                "Upgrade-Insecure-Requests: 1\r\n"
                "Sec-Fetch-Dest: document\r\n"
                "Sec-Fetch-Mode: navigate\r\n"
                "Sec-Fetch-Site: same-origin\r\n"
                "Sec-Fetch-User: ?1\r\n"
                "\r\n"
                "username=tomas&password=dougan";

    request_init(&req);
    status = parse_request(&req, raw);

    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_INT(POST, req.method);
    TEST_ASSERT_EQUAL_STRING("/login.html", req.uri);
    TEST_ASSERT_EQUAL_INT(APP_XFORM, req.content_type);
    TEST_ASSERT_EQUAL_INT(30, req.content_len);
    TEST_ASSERT_EQUAL_STRING("username=tomas&password=dougan", req.content);
}

/*********************
 * basic_handle_post *
 *********************/

void
basic_handle_post()
{
    struct request req;
    int status;
    char* res;
    char* html = "<div id=\"username\" ></div> <div id=\"password\" ></div>";

    request_init(&req);
    req.method = POST;
    req.content_type = APP_XFORM;
    req.content_len = 30;
    req.content = "username=tomas&password=dougan";
    handle_post(&req, html, &res);

    TEST_ASSERT_EQUAL_STRING("<div id=\"username\" >tomas</div> "
                             "<div id=\"password\" >dougan</div>", res);

}

/*********************************************************************
 *                                                                   *
 *                              main                                 *
 *                                                                   *
 *********************************************************************/

/********
 * main *
 ********/

int
main() 
{
    UNITY_BEGIN();
    RUN_TEST(basic_get);
    RUN_TEST(basic_split);
    RUN_TEST(split_on_html);
    RUN_TEST(put_with_headers);
    RUN_TEST(basic_handle_post);
    return UNITY_END();
}


