#include "http.h"

/*********************************************************************
 *                                                                   *
 *                            initializers                           *
 *                                                                   *
 *********************************************************************/

/****************
 * init_request *
 ****************/

void 
init_request(struct request* req);

/*****************
 * init_response *
 *****************/

int 
init_response(struct response* resp);

 /********************************************************************
 *                                                                   *
 *                             conversion                            *
 *                                                                   *
 *********************************************************************/

/*****************
 * parse_request *
 *****************/

int 
parse_request(struct request* req, char* data, int len);


/*****************
 * make_response *
 *****************/

void 
make_response(struct response* resp, char** data, int* len);

/*********************************************************************
 *                                                                   *
 *                            destructors                            *
 *                                                                   *
 *********************************************************************/

/****************
 * free_request *
 ****************/

void 
free_request(struct request* req);

/*****************
 * free_response *
 *****************/

void 
free_response(struct response* resp);

