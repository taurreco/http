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
    status = parse_request(&req, "GET / HTTP/1.0", 100);

    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_INT(GET, req.method);
    TEST_ASSERT_EQUAL_STRING("/index.html", req.uri);
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
    return UNITY_END();
}


