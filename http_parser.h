#ifndef _PARSER_H__
#define _PARSER_H__

//http syntax: https://httpwg.org/specs/rfc7230.html#core.rules

#define REQUEST_OK (1)
#define INVALID_REQUEST (-1)
#define UNFINISHED_REQUEST (-2)

#define URI_MAX_LENGTH 256

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
    char uri[URI_MAX_LENGTH];
    http_version version;    
}http_request_t;



int parse_request(char *request_buf, unsigned int buf_len, http_request_t * request);


#endif