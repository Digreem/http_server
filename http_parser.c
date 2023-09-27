#include "http_parser.h"
#include "http_parser.h"
#include <stdbool.h>

//http syntax: https://httpwg.org/specs/rfc7230.html#core.rules

// "POST /foo HTTP/1.1\r\n"
// request-line   = method SP request-target SP HTTP-version CRLF

typedef struct http_msg_integrity_flags
{
    bool is_method;
    bool is_uri;
    bool is_http_version;
};

int parse_request(char *request_buf, unsigned int buf_len, http_request_t * request)
{
    // Parse method
    // Parse URI
    // Parse http_version
    unsigned int pos = 0;
    for( ;pos < buf_len; pos++)
    {

    }
}