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
    PS_HEADER_NAME_END,
    PS_HEADER_VALUE,
    PS_HEADER_VALUE_END,

} parser_state_t;

typedef struct parser
{
    unsigned curr_pos;
    parser_state_t state;
}parser_t;

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
 //  unsigned length = uri_end - uri_start;

    // if(length >= URI_MAX_LENGTH)
    //     return INSUFFICIENT_BUF;

    if(request_buf[uri_start] != '/')
        return INVALID_REQUEST;

    request->uri.str = request_buf + uri_start;
    request->uri.len = uri_end - uri_start;
    
    // memmove(request->uri, request_buf + uri_start, length);
    // request->uri[length] = '\0';

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

static int parse_start_line(http_request_t* request_ptr, parser_t* pr)
{
    int retval = INVALID_REQUEST;

    unsigned pos = pr->curr_pos;
    unsigned pos_method_begin = 0;
    unsigned pos_uri_begin = 0;
    unsigned pos_version_begin = 0;
    char *request_buf = request_ptr->msg_buf.buff_ptr;

    bool parse_end = false;

    for( ;pos < request_ptr->msg_buf.buff_len; pos++)
    {
        if(true == parse_end)
            break;

        switch(pr->state)
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
                    pr->state = PS_METHOD;
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
                    pr->state = PS_URI;
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

                    pr->state = PS_HTTP_VERSION;
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
                    pr->state = PS_HEADER_BEGIN;
                    parse_end = true;
                    continue;
                }
                else
                {
                    continue;
                }         
                break;

            default:
                break;
        }
    }
    if(true == parse_end)
    {
        retval = REQUEST_OK;
        pr->curr_pos = pos;
    }
    else
    {
        retval = UNFINISHED_REQUEST;
    }

    return retval; 
}

static int parse_header_content_type(unsigned val_start, unsigned val_end, http_request_t* r_ptr)
{
    r_ptr->headers_checkbox[ContentType] = true;
    r_ptr->hd_content_type.str = r_ptr->msg_buf.buff_ptr + val_start;
    r_ptr->hd_content_type.len = val_end - val_start;

    if(r_ptr->hd_content_type.len < 0)
        return INVALID_REQUEST;
    else 
        return REQUEST_OK;
}
static parse_header_content_length(unsigned val_start, unsigned val_end, http_request_t* r_ptr)
{
    r_ptr->headers_checkbox[ContentLength] = true;
    
    if(val_end - val_start < 0)
        return INVALID_REQUEST;
    
}

static http_headers_t recognize_header_name(unsigned name_start, unsigned name_end, char* msg_buff)
{
    char name_cstr_buf[HTTP_HD_NAME_MAX_LENGTH];
    unsigned length = name_end - name_start;
    http_headers_t header = HeaderUndefined;

    if (length >= HTTP_HD_NAME_MAX_LENGTH)
        return HeaderUndefined;

    // transform to c-style string
    memcpy(name_cstr_buf, msg_buff + name_start, length);
    name_cstr_buf[length] = '\0';

    for (unsigned i = 0; i < HttpHeadersCount; i++)
    {
        if(0 == strcmp(name_cstr_buf, supported_http_headers[i]))
        {
            header = (http_headers_t)i;
            break;
        }
    }

    return header;
}

static bool is_header_name_symbol(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '-'))
        return true;
    else 
        return false;
}

//doc: https://httpwg.org/specs/rfc7230.html#header.fields
//header-field   = field-name ":" OWS field-value OWS
//OWS = *( SP / HTAB )
//      ; optional whitespace

static int parse_header(http_request_t* request_ptr, parser_t *pr)
{
    int retval = INVALID_REQUEST;
    unsigned pos = pr->curr_pos;

    unsigned pos_header_name_begin = pos;
    unsigned pos_header_name_end = 0;
    unsigned pos_header_val_begin = 0;
    unsigned pos_header_val_end = 0;

    char *msg_buff = request_ptr->msg_buf.buff_ptr;  

    bool parse_end = false;

    for( ;pr->curr_pos < request_ptr->msg_buf.buff_len; pr->curr_pos++)
    {
        if(true == parse_end)
            break;

        char cc = msg_buff[pr->curr_pos]; //current symbol

        switch(pr->state)
        {
            case PS_HEADER_BEGIN:
                if(is_header_name_symbol(cc))
                {
                    continue;
                }
                else if (':' == cc)
                {
                    pr->state = PS_HEADER_NAME_END;
                    pos_header_name_end = pr->curr_pos;
                }
                else
                {
                    return INVALID_REQUEST;
                }
                break;

            case PS_HEADER_NAME_END:
                if(' ' == cc || '\t' == cc) // skip OWS
                {
                    continue;
                }
                else if('\r' == cc || '\n' == cc)
                {
                    return INVALID_REQUEST;
                }
                else
                {
                    pr->state = PS_HEADER_VALUE;
                    pos_header_val_begin = pr->curr_pos;
                }
                break;
            case PS_HEADER_VALUE:
                if(' ' == cc || '\t' == cc || '\n' == cc || '\r' == cc)
                {
                    pr->state = PS_HEADER_VALUE_END;
                    pos_header_val_end = pr->curr_pos;
                }
                else
                {
                    continue;
                }
                break;
            case PS_HEADER_VALUE_END:
                if(' ' == cc || '\t' == cc) // skip OWS
                {
                    continue;
                }
                else if('\r' == msg_buff[pr->curr_pos - 1] && '\n' == msg_buff[pr->curr_pos])
                {
                    parse_end = true;
                    pr->state = PS_HEADER_BEGIN;
                }
                else
                {
                    return INVALID_REQUEST; 
                }    
                break;
            default:
                break;
        }
    }

    if(parse_end)
    {
        // recognize or ignore header 
        http_headers_t header = recognize_header_name(pos_header_name_begin, pos_header_name_end, msg_buff);
        retval = REQUEST_OK;
        if (header == ContentType)
        {
            request_ptr->headers_checkbox[ContentType] = true;
            request_ptr->hd_content_type.str = pos_header_val_begin;
            request_ptr->hd_content_type.len = pos_header_val_end - pos_header_val_begin;
        }
        else if (header == ContentLength)
        {

        }
    }
    else
    {
        retval = UNFINISHED_REQUEST;
    }

    return retval;
}

static int parse_payload(http_request_t* request_ptr, parser_state_t* p_state, unsigned pos)
{
    return 0;
}

// doc: https://httpwg.org/specs/rfc7230.html#http.message
//  HTTP-message   = start-line
//                   *( header-field CRLF )
//                   CRLF
//                   [ message-body ]

int parse_request(http_request_t* request_ptr)
{
    int retval = INVALID_REQUEST;

    parser_t parser;
    bool headers_end = false;
    parser.state = PS_PARSER_START;
    parser.curr_pos = 0;

    // parse start line
    retval = parse_start_line(request_ptr, &parser);
    if(retval != REQUEST_OK)
        return retval; 

    // parse headers
    request_ptr->headers_checkbox[ContentLength] = false;
    request_ptr->headers_checkbox[ContentType] = false;

    while(1)
    {
        // check "\r\n" symbols signaling about end of headers field
        if(request_ptr->msg_buf.buff_len <= parser.curr_pos + 1)
            return INVALID_REQUEST;
        else
        {
            if(request_ptr->msg_buf.buff_ptr[parser.curr_pos] == '\r' &&
               request_ptr->msg_buf.buff_ptr[parser.curr_pos + 1] == '\n')
            {
                headers_end = true;
                break;

            }
        }
        
        // parse header line
        retval = parse_header(request_ptr, &parser);
        if(retval != REQUEST_OK)
            return retval;

        //return retval; 

        // decide if we have to parse http payload
    }
    

    return retval; 
}

//transform rstr to C-style string 
/** @return length of output string or -1 in case of buffer length is insufficient */
int rstr_to_cstr(rstr_t* source, char* dest_buf, unsigned buf_length)
{
    if (buf_length <= (unsigned)source->len)
        return -1;

    memcpy(dest_buf, source->str, source->len);
    dest_buf[source->len] = '\0';
    return source->len;
}