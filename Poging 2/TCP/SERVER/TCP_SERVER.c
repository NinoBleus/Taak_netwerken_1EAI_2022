#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

//PRINT CONNECTION INFO
void print_ip_address( unsigned short family, struct sockaddr * ip ) {
	void * ip_address;
	char * ip_version;
	char ip_string[INET6_ADDRSTRLEN];

	if( family == AF_INET )
	{ // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)ip;
		ip_address = &(ipv4->sin_addr);
		ip_version = "IPv4";
	}
	else
	{ // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ip;
		ip_address = &(ipv6->sin6_addr);
		ip_version = "IPv6";
	}

	inet_ntop( family, ip_address, ip_string, sizeof ip_string );
	printf( "%s -> %s\n", ip_version, ip_string );
}

void ai_print_ip_address( struct addrinfo * ip ) {
	print_ip_address( ip->ai_family, ip->ai_addr );
}
//PRINT CONNECTION INFO

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main( int argc, char * argv[] )
{
//START WINSOCK API
	WSADATA wsaData; //WSAData wsaData; //Could be different case
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 ) // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}
//START WINSOCK API

//SETUP SOCKET
	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE; // use ANY address for IPv4 and IPv6

	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( NULL, "24042", &internet_address_setup, &result_head );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 2 );
	}
//SETUP SOCKET

//CREATE SOCKET & LISTEN
	int internet_socket;

	result_item = result_head; //take first of the linked list
	while( result_item != NULL ) //while the pointer is valid
	{
		internet_socket = socket( result_item->ai_family, result_item->ai_socktype, result_item->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			int return_code;
			return_code = bind( internet_socket, result_item->ai_addr, result_item->ai_addrlen );
			if( return_code == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				int listen_return;
				listen_return = listen( internet_socket, 5 );
				if( listen_return == -1 )
				{
					close( internet_socket );
					perror( "listen" );
				}
				else
				{
					printf( "listen on " );
					ai_print_ip_address( result_item );
					break; //stop running through the linked list
				}
			}
		}
		result_item = result_item->ai_next; //take next in the linked list
	}
	if( result_item == NULL )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 5 );
	}
	freeaddrinfo( result_head ); //free the linked list
//CREATE SOCKET & LISTEN

//SET FILE DESCRIPTOR
  fd_set master;
  fd_set read_fds;
  int fdmax = 0;
//SET FILE DESCRIPTOR

//CLEAR FD
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
//CLEAR FD

//ADD INTERNET_SOCKET TO MASTER SET
  FD_SET(internet_socket, &master);
//ADD INTERNET_SOCKET TO MASTER SET

//SET MAX FD LENGHT
  fdmax = internet_socket;
//SET MAX FD LENGHT

//CLIENT CONNECTION STORAGE
  struct sockaddr_storage client_ip_address;
  socklen_t client_ip_address_length;
  char remoteIP[INET6_ADDRSTRLEN];
//CLIENT CONNECTION STORAGE

//CLIENT HANDELING
  int i, j = 0;
  int newfd = 0;

  int number_of_bytes_received = 0;
	char buffer[1000] = "\0";
  char new_con[100] = "\0";

  while (1) {
    read_fds = master;
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
        perror("select");
        exit(4);
    }

    //LOOP THROUGH CONNECTIONS
    for(i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fds)) {
            if (i == internet_socket) {

                //NEW CONNECTION
                client_ip_address_length = sizeof(client_ip_address);
                newfd = accept(internet_socket,
                    (struct sockaddr *)&client_ip_address,
                    &client_ip_address_length);

                if (newfd == -1) {
                    perror("accept");
                }
                else {
                    FD_SET(newfd, &master);
                    if (newfd > fdmax) {
                        fdmax = newfd;
                    }
                    sprintf(new_con, "new connection from %s on socket %d\n", inet_ntop(client_ip_address.ss_family,
                           get_in_addr((struct sockaddr*)&client_ip_address),
                           remoteIP, INET6_ADDRSTRLEN), newfd);
                    printf("%s", new_con);


                      for(j = 0; j <= fdmax; j++) {
                          if (FD_ISSET(j, &master)) {
                              if (j != internet_socket && j != i) {
                                 if (send(j, new_con, strlen(new_con), 0) == -1) {
                                     perror("send");
                                 }
                             }
                         }
                     }
                }
            }
            else {
                //DATA HANDLING
                if ((number_of_bytes_received = recv(i, buffer, sizeof(buffer), 0)) <= 0) {
                    if (number_of_bytes_received == 0) {
                        printf("selectserver: socket %d hung up\n", i);
                    }
                    else {
                        perror("recv");
                    }
                    close(i);
                    FD_CLR(i, &master);
                }

                else {
                    for(j = 0; j <= fdmax; j++) {
                        if (FD_ISSET(j, &master)) {
                            if (j != internet_socket && j != i) {
                                if (send(j, buffer, number_of_bytes_received, 0) == -1) {
                                    perror("send");
                                }
                            }
                        }
                    }
                }
            } //DATA HANDLING
        } //NEW CONNECTION
    } //LOOP THROUGH CONNECTIONS
  } //CLIENT HANDELING
	return 0;
}
