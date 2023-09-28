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

typedef enum parser_state
{
    PS_PARSER_START,
    PS_METHOD,
    PS_URI,
    PS_HTTP_VERSION,

} parser_state_t;

int parse_request(char *request_buf, unsigned int buf_len, http_request_t * request)
{
    int retval = INVALID_REQUEST;
    unsigned int pos = 0;
    unsigned int pos_method_begin = 0;
    unsigned int pos_uri_begin = 0;
    unsigned int pos_version_begin = 0;

    parser_state_t p_state = PS_PARSER_START;

    bool parse_end = false;

    for( ;pos < buf_len; pos++)
    {
        if(true == parse_end)
            break;

        switch(p_state)
        {
            case PS_PARSER_START:
                if(' ' == request_buf[pos] || '\t' == request_buf[pos])
                {
                    continue;
                }
                else if('A' <= request_buf[pos] && 'Z' >= request_buf[pos])
                {
                    pos_method_begin = pos;
                    p_state = PS_METHOD;
                    continue;
                }
                else
                {
                    return INVALID_REQUEST;
                }
                break;
                
            case PS_METHOD:
                if(' ' == request_buf[pos])
                {
                    // call parser_detect_method()
                    p_state = PS_URI;
                    pos_uri_begin = pos + 1;
                    continue;
                }
                else if('A' <= request_buf[pos] && 'Z' >= request_buf[pos])
                {
                    continue;
                }
                else
                {
                    return INVALID_REQUEST;
                }
                break;

            case PS_URI:
                if(' ' == request_buf[pos])
                {
                    // call parser_detect_uri()
                    p_state = PS_HTTP_VERSION;
                    pos_version_begin  = pos + 1;
                    continue;
                }
                else if ('\n' == request_buf[pos] || '\r' == request_buf[pos])
                    return INVALID_REQUEST;
                else
                    continue;
                break;

            case PS_HTTP_VERSION:
                if('\n' == request_buf[pos] && '\r' == request_buf[pos - 1])
                {
                    // call parser_detect_http_version()
                    //p_state = PS_HTTP_V_END;
                    parse_end = true;
                    continue;
                }
                else
                {
                    continue;
                }         
                break;
        }
    }
    if(true == parse_end)
        retval = REQUEST_OK;
    else
        retval = UNFINISHED_REQUEST;

    return retval; 
}