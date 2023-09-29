#ifndef _PARSER_H__
#define _PARSER_H__

//http syntax: https://httpwg.org/specs/rfc7230.html#core.rules

#define REQUEST_OK (1)
#define INVALID_REQUEST (-1)
#define UNFINISHED_REQUEST (-2)
#define INSUFFICIENT_BUF (-3)

#define URI_MAX_LENGTH 256

/* HTTP protocol version */
typedef struct {
  unsigned short http_major;
  unsigned short http_minor;
} http_version;

/* Avaliable http request methods*/
typedef enum http_method{
  // Supported methods
  HTTP_DELETE = 0,
  HTTP_GET,
  HTTP_POST,
  // Supported methods count
  HTTP_METHODS_COUNT,
  // If method undefined
  HTTP_UNDEFINED
} http_method_t;
 
/* http request structure*/ 
typedef struct http_request
{
    http_method_t method;
    char uri[URI_MAX_LENGTH];
    http_version version;    
}http_request_t;



int parse_request(char *request_buf, unsigned int buf_len, http_request_t * request);


#endif