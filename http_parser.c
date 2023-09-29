#include "http_parser.h"
#include "http_parser.h"
#include <stdbool.h>
#include <ctype.h>
#include <string.h>



//http syntax: https://httpwg.org/specs/rfc7230.html#core.rules

// "POST /foo HTTP/1.1\r\n"
// request-line   = method SP request-target SP HTTP-version CRLF

#define HTTP_METHOD_MAX_LENGTH 16
#define HTTP_HD_NAME_MAX_LENGTH 32
#define HTTP_HD_VAL_MAX_LENGTH 128

// typedef struct http_msg_integrity_flags
// {
//     bool is_method;
//     bool is_uri;
//     bool is_http_version;
// };

typedef enum parser_state
{
    PS_PARSER_START,
    PS_METHOD,
    PS_URI,
    PS_HTTP_VERSION,
    PS_HEADER_BEGIN,

} parser_state_t;

// All reference string stored in lowercase and comparison performed also in lowercase
const char supported_http_methods[HTTP_METHODS_COUNT][HTTP_METHOD_MAX_LENGTH] = 
{  
    [HTTP_DELETE] = "delete",
    [HTTP_GET]    = "get",
    [HTTP_POST]   = "post"          
};

//TODO  ??? Find out if name os case-sensitive
const char supported_http_headers[HttpHeadersCount][HTTP_HD_NAME_MAX_LENGTH] = 
{

    [ContentType] =   "Content-Type",
    [ContentLength] = "Content-Length"                                  
};
    

//Converting null terminated ASCII C-style string to lowercase
static void ascii_str_tolower(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if(str[i] >= 'A' && str[i] <= 'Z')
            str[i] = (char)tolower((int)(str[i]));
    }
}

static http_method_t parse_method(char* request_buf, unsigned method_start, unsigned method_end)
{
    char method_str[HTTP_METHOD_MAX_LENGTH] = "";
    http_method_t result_method = HTTP_UNDEFINED;

    unsigned length = method_end - method_start;
    
    if(length >= HTTP_METHOD_MAX_LENGTH)
        return HTTP_UNDEFINED;

    memmove(method_str, request_buf + method_start, length);
    method_str[length] = '\0';
    // transorm to lowercase before performing string comparison
    ascii_str_tolower(method_str);
    
    for(unsigned i = 0; i < HTTP_METHODS_COUNT; i++)
    {
        if(0 == strcmp(method_str, supported_http_methods[i]))
        {
            result_method = (http_method_t)i;
            break;
        }
    }
    return result_method;
}

static int parse_uri(char *request_buf, unsigned uri_start, unsigned uri_end, http_request_t* request)
{
    unsigned length = uri_end - uri_start;

    if(length >= URI_MAX_LENGTH)
        return INSUFFICIENT_BUF;

    if(request_buf[uri_start] != '/')
        return INVALID_REQUEST;
    
    memmove(request->uri, request_buf + uri_start, length);
    request->uri[length] = '\0';

    return REQUEST_OK;
}

// documentation: https://httpwg.org/specs/rfc7230.html#http.version
//
//    HTTP-version  = HTTP-name "/" DIGIT "." DIGIT
//    HTTP-name     = %x48.54.54.50 ; "HTTP", case-sensitive 

static int parse_http_version(char *request_buf, unsigned version_start, unsigned version_end, http_request_t* request_ptr)
{
    unsigned length = version_end - version_start;
    if(length != 8) //deteermined size of protocol version in http message
        return INVALID_REQUEST;

    if(strncmp(request_buf + version_start, "HTTP/", 5) != 0 || request_buf[version_start + 6] != '.')
        return INVALID_REQUEST;

    request_ptr->version.http_major = (unsigned short)(request_buf[version_start + 5] - '0');
    if(request_ptr->version.http_major < 0 || request_ptr->version.http_major > 9)
        return INVALID_REQUEST;

    request_ptr->version.http_minor= (unsigned short)(request_buf[version_start + 7] - '0');
    if(request_ptr->version.http_minor < 0 || request_ptr->version.http_minor > 9)
        return INVALID_REQUEST;    
    
    return REQUEST_OK;
}

static int parse_start_line(http_request_t* request_ptr, parser_state_t* p_state)
{
    return 0;
}

static int parse_headers(http_request_t* request_ptr, parser_state_t* p_state)
{
    return 0;
}

static int parse_payload(http_request_t* request_ptr, parser_state_t* p_state)
{
    return 0;
}

int parse_request(http_request_t* request_ptr)
{
    int retval = INVALID_REQUEST;
    unsigned int pos = 0;
    unsigned int pos_method_begin = 0;
    unsigned int pos_uri_begin = 0;
    unsigned int pos_version_begin = 0;

    char *request_buf = request_ptr->msg_buf.buff_ptr;

    parser_state_t p_state = PS_PARSER_START;

    bool parse_end = false;

    for( ;pos < request_ptr->msg_buf.buff_len; pos++)
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
                else if(('A' <= request_buf[pos] && 'Z' >= request_buf[pos]) ||
                        ('a' <= request_buf[pos] && 'z' >= request_buf[pos]))
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
                    request_ptr->method = parse_method(request_buf, pos_method_begin, pos);
                    if(HTTP_UNDEFINED == request_ptr->method)
                        return INVALID_REQUEST;
                    p_state = PS_URI;
                    pos_uri_begin = pos + 1;
                    continue;
                }
                else if(('A' <= request_buf[pos] && 'Z' >= request_buf[pos]) ||
                        ('a' <= request_buf[pos] && 'z' >= request_buf[pos]))
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
                    retval = parse_uri(request_buf, pos_uri_begin, pos, request_ptr);
                    if(REQUEST_OK != retval)
                        return retval;

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
                if('\r' == request_buf[pos - 1] && '\n' == request_buf[pos])
                {
                    retval = parse_http_version(request_buf,pos_version_begin, pos - 1, request_ptr);
                    if(REQUEST_OK != retval)
                        return retval;
                    p_state = PS_HEADER_BEGIN;
                    parse_end = true;
                    continue;
                }
                else
                {
                    continue;
                }         
                break;
            case PS_HEADER_BEGIN:
            {

            }
        }
    }
    if(true == parse_end)
        retval = REQUEST_OK;
    else
        retval = UNFINISHED_REQUEST;

    return retval; 
}