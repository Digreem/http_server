#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"
#include "http_parser.h"

#define PORT 8080
#define MAX_LENGTH  100
#define SOCKET_OPT_ENABLE 1
#define ECHO_MODE 1
#define MAX_HEADER_SIZE 32
#define RESPONSE_BUF_SIZE MAX_HEADER_SIZE + MAX_LENGTH

char msg_buff[MAX_LENGTH];
char response_buff[RESPONSE_BUF_SIZE];

char http_msg_test[] = "POST /foo HTTP/1.1\r\n"
	"User-Agent: curl/7.35.0\r\n"
	"Host: example.com\r\n"
	"Accept: */*\r\n"
	"Content-Length: 3\r\n"
	"Content-Type: text/plain\r\n"
	"\r\n"
	"foo";

void communicate(int sockfd)
{
    int n;
    // infinite loop for chat
    for (;;) {
        bzero(msg_buff, MAX_LENGTH);
        bzero(response_buff, RESPONSE_BUF_SIZE);

        // read the message from client and copy it in buffer
        read(sockfd, msg_buff, sizeof(msg_buff));

		// ----------------------------------------------------
		// thinks about dynamic reading with fixed window buffer
		// like: https://github.com/chendotjs/lotos/blob/master/src/request.c || Func: request_recv()
		//-----------------------------------------------------

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


        // print buffer which contains the client contents
        printf("From client: %s", msg_buff);

        if(!ECHO_MODE)
        {
        	n = 0;
        	// copy server message in the buffer
        	printf("Enter the string to send : ");
        	fflush(stdin);
        	while ((response_buff[n++] = getchar()) != '\n')
            ;
        }
        else
        {
        	strncpy(response_buff, "\n\r *** SERVER ECHO ***\n\r ", MAX_HEADER_SIZE);
        	strncat(response_buff, msg_buff, MAX_LENGTH);
        }

        // and send that buffer to client
        write(sockfd, response_buff, strlen(response_buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", msg_buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
	// Listening & connection sockets
	int listen_sock, connect_sock;
	// Socket address structure
	struct sockaddr_in servaddr, clientaddr;

	printf("Hello from server!!!\n");

	printf("%s",http_msg_test);

	//----------- test parser------------------ 
	http_request_t request;
	int parse_res = parse_request(http_msg_test, strlen(http_msg_test), &request);
	//-----------------------------------------

	getchar();  // Debug purpose
	return 0; // Debug purpose

	// hashtable test //-----------------------------
	struct hashtable *h_table = hashtable_create(128, NULL);
	hashtable_destroy(h_table);
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
	else
		printf("# Server listening for incoming connections...\n");

	// Accept the data packet from client and verification
	socklen_t client_len = sizeof(clientaddr);
	connect_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &client_len);
	if (connect_sock< 0)
	{
		printf("# Error: server accept failed #\n");
		exit(0);
	}
	else
		printf("# Error: server acccepted the client #\n");

	communicate(connect_sock);

	close(listen_sock);
	printf("# Listening socket closed #\n");
	close(connect_sock);
	printf("# Connection socket closed #\n");
	return 0;
}
