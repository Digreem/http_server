#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "http_parser.h"

#define PORT 8080
#define MAX_MSG_LENGTH  1024
#define SOCKET_OPT_ENABLE 1
#define ECHO_MODE 0
#define MAX_HEADER_SIZE 32
#define RESPONSE_BUF_SIZE MAX_HEADER_SIZE + MAX_MSG_LENGTH

char msg_buff[MAX_MSG_LENGTH];
char response_buff[RESPONSE_BUF_SIZE];

char http_msg_test[] = "POST /foo/456/&&8*#@@/aa HTTP/1.1\r\n"
	"User-Agent: curl/7.35.0\r\n"
	"Host: example.com\r\n"
	"Accept: */*\r\n"
	"Content-Length: 3478654\r\n"
	"Content-Type: text/plain\r\n"
	"\r\n"
	"foo";

static void print_parsed_request(http_request_t* request)
{
	char uri_str[URI_MAX_LENGTH];
	char hd_val[HD_VAL_MAX_LENGTH];
	printf("\tmethod: ");
	switch(request->method)
	{
		case HTTP_GET:
			printf("GET\n");
			break;
		
		case HTTP_POST:
			printf("POST\n");
			break;  

		case HTTP_DELETE:
			printf("DELETE\n");
			break; 
		default:
			break;
	}
	if (rstr_to_cstr(&(request->uri), uri_str, URI_MAX_LENGTH) >= 0)	
		printf("\turi: %s\n", uri_str);
	else
		printf("\turi: ERROR TRANSFORMIN R_STR TO C_STR"); 

	printf("\thttp version: %hu.%hu\n\n", request->version.http_major, request->version.http_minor);

	if(request->headers_checkbox[ContentType] == true)
	{
		if (rstr_to_cstr(&(request->hd_content_type), hd_val, HD_VAL_MAX_LENGTH) >= 0)	
			printf("\tHD{Content-Type}: %s\n", hd_val);
		else
			printf("\tHD{Content-Type}: ERROR TRANSFORMIN R_STR TO C_STR"); 
	}
	if(request->headers_checkbox[ContentLength] == true)
		printf("\tHD{Content-Length}: %u", request->hd_content_length);

}

void handle_connection(int sockfd)
{
    //int n;
	http_request_t request;

	request.msg_buf.buff_ptr = msg_buff;
	request.msg_buf.buff_len = MAX_MSG_LENGTH;

	memset(request.msg_buf.buff_ptr, 0, request.msg_buf.buff_len);

	// ----------------------------------------------------
	// thinks about dynamic reading with fixed window buffer
	// like: https://github.com/chendotjs/lotos/blob/master/src/request.c || Func: request_recv()
	//-----------------------------------------------------
	read(sockfd, request.msg_buf.buff_ptr, request.msg_buf.buff_len);

	// print buffer which contains the client contents
	printf("From client: %s\n\n", msg_buff);

	// -----------------------------------------------------
	// Add here an html request parser and extract following fields:
	// - Start line:
	// - - HTTP Method
	// - - URI
	// - - HTTP version
	// - Headers:
	// - - Content-type
	// - - Content-Length
	// - Body
	// -----------------------------------------------------
	// link: https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages
	// -----------------------------------------------------
	
	//int parse_res = parse_request(msg_buff, strlen(http_msg_test), &request);


	// -----------------------------------------------------
	// Add hashing to lookup resources:
	// Key  -- URI
	// Data -- resource structure
	// -----------------------------------------------------

	// -----------------------------------------------------
	// Add file reading from:
	// https://github.com/bloominstituteoftechnology/C-Web-Server/blob/master/src/file.c
	//------------------------------------------------------
	// Think out about how to manage both text and binary files
	//------------------------------------------------------


	if(REQUEST_OK == parse_request(&request))
	{
		printf("### http message received:\n");
		print_parsed_request(&request);
		printf("\n\n");

	}

	else
	{
		printf("ERROR while parsing http msg:\n");
	}

	// write(sockfd, response_buff, strlen(response_buff));

}

int main(int argc, char *argv[])
{
	// Listening & connection sockets
	int listen_sock, connect_sock;
	// Socket address structure
	struct sockaddr_in servaddr;

	printf("Hello from server!!!\n");

	//printf("%s",http_msg_test);

	//----------- test parser------------------ 
	http_request_t request;
	request.msg_buf.buff_ptr = http_msg_test;
	request.msg_buf.buff_len = sizeof(http_msg_test);

	int parse_res = parse_request(&request);
	if(parse_res == REQUEST_OK)
		print_parsed_request(&request);
	//-----------------------------------------

	getchar();  // Debug purpose
	return 0; // Debug purpose

	// hashtable test //-----------------------------
	// struct hashtable *h_table = hashtable_create(128, NULL);
	// hashtable_destroy(h_table);
	//-----------------------------------------------


	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == -1)
	{
		printf("# Error: socket creation failed #\n");
		exit(0);
	}
	else
		printf("# Socket successfully created #\n");

	// assign IP, PORT
	memset (&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// set reusing socket option
	const int opt  = SOCKET_OPT_ENABLE;
	if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
	{
		printf("# Error on setting SO_REUSEADDR option to port #%d\n", PORT);
		exit(0);
	}

	// Binding newly created socket to given IP and verification
	if ((bind(listen_sock, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
		printf("# Error: socket bind failed...\n #");
		exit(0);
	}
	else
		printf("# Socket successfully binded #\n");

	// Now server is ready to listen
	if ((listen(listen_sock, 5)) != 0) {
		printf("# Error: Listen failed #\n");
		exit(0);
	}		

	// serever will open new connection for each request
	while(1)
	{
		struct sockaddr_in clientaddr;
		// Accept the data packet from client and verification
		socklen_t client_len = sizeof(clientaddr);

		printf("\n# Server listening for incoming connections on port %d...\n", PORT);

		connect_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &client_len);
		if (connect_sock< 0)
		{
			printf("# Error: server accept failed #\n");
			exit(0);
		}
		else
			printf("# Server acccepted the client #\n");

		handle_connection(connect_sock);

		close(connect_sock);
		printf("# Connection socket closed #\n");
	}

	close(listen_sock);
	printf("# Listening socket closed #\n");

	return 0;

}
