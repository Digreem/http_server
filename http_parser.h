#ifndef _PARSER_H__
#define _PARSER_H__

//http syntax: https://httpwg.org/specs/rfc7230.html#core.rules


/* HTTP protocol version */
typedef struct {
  unsigned short http_major;
  unsigned short http_minor;
} http_version;

/* Avaliable http request methods*/
typedef enum http_method{
  HTTP_DELETE,
  HTTP_GET,
  HTTP_POST
} http_method;
 
/* http request structure*/ 
typedef struct http_request
{
    http_method method;
    char uri[256];
    http_version version;


}http_request_t;


#endif