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
    init_request(&req);
    parse_request(&req, "GET / HTTP/1.0");

    TEST_ASSERT_INT_EQUAL(GET, req.method);
    TEST_ASSERT_STR_EQUAL("pages/index.html", req.uri.raw);
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
    RUN_TEST(basic);
    return UNITY_END();
}


