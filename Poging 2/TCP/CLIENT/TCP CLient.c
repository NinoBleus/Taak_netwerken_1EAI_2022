#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>

#include <stdio.h>
#include <unistd.h>

int thread_Stop=0;

void print_ip_address( struct addrinfo * ip )
{
	void * ip_address;
	char * ip_version;
	char ip_string[INET6_ADDRSTRLEN];

	if( ip->ai_family == AF_INET )
	{ // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)ip->ai_addr;
		ip_address = &(ipv4->sin_addr);
		ip_version = "IPv4";
	}
	else
	{ // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ip->ai_addr;
		ip_address = &(ipv6->sin6_addr);
		ip_version = "IPv6";
	}

	inet_ntop( ip->ai_family, ip_address, ip_string, sizeof ip_string );
	printf( "%s -> %s\n", ip_version, ip_string );
}

void *datareceive(void *internet_socket)
{
  int receive_Socket=(intptr_t) internet_socket;
  int receivedBytes=0;
  char buffer[1000]="\0";

  while(thread_Stop == 0)
  {
    receivedBytes = recv(receive_Socket, buffer, sizeof(buffer), 0);
    if (receivedBytes == -1)
    {
      perror("recv");
    }
    buffer[receivedBytes]='\0';
    if (strlen(buffer)>0)
    {
      printf("%s\n", buffer);
    }
  }
  pthread_exit(NULL);
  return NULL;
}

int main( int argc, char * argv[] )
{
	WSADATA wsaData; //WSAData wsaData; //Could be different case
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 ) // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}//oproepen API

  char serIP[45]="\0";
  char poort[5]="\0";

  printf("Geef IP in:\n");
  scanf("%s", serIP);
  fflush(stdin);
  //ip opgeven
  printf("Geef Poort in:\n");
  scanf("%s", poort);
  fflush(stdin);
  //poort opgeven

	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	internet_address_setup.ai_socktype = SOCK_STREAM;

	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( serIP, poort, &internet_address_setup, &result_head );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 2 );
	}

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
			printf( "Connecting to " );
			print_ip_address( result_item );

			int connect_return;
			connect_return = connect( internet_socket, result_item->ai_addr, result_item->ai_addrlen );
			if( connect_return == -1 )
			{
				perror( "connect" );
				close( internet_socket );
			}
			else
			{
				printf( "Connected\n" );
				break; //stop running through the linked list
			}
		}
		result_item = result_item->ai_next; //take next in the linked list
	}
	if( result_item == NULL )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 4 );
	}
	freeaddrinfo( result_head ); //free the linked list

  //start pthread

  pthread_t receive;
  if (pthread_create(&receive, NULL, datareceive, (void * )(intptr_t)internet_socket)!=0)
  {
    printf("Error\n");
    return 0;
  }

  char datasend[1000]="\0";
  int number_of_bytes_send = 0;

  while(1)
  {
    printf("Geef Bericht in:\n" );
    fflush(stdin);
    gets(datasend);
    if (strcmp(datasend, "/exit")==0)
    {
    break;
    }
    strcat(datasend, "\n");
    number_of_bytes_send = send( internet_socket, datasend, strlen(datasend), 0 );
    if( number_of_bytes_send == -1 )
    {
      printf( "errno = %d\n", WSAGetLastError() ); //https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
      perror( "send" );
    }
  }

	int shutdown_return;
  thread_Stop = 1;
	shutdown_return = shutdown( internet_socket, SD_SEND ); //Shutdown Send == SD_SEND ; Receive == SD_RECEIVE ; Send/Receive == SD_BOTH ; https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable --> Linux : Shutdown Send == SHUT_WR ; Receive == SHUT_RD ; Send/Receive == SHUT_RDWR
	if( shutdown_return == -1 )
	{
		printf( "errno = %d\n", WSAGetLastError() ); //https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		perror( "shutdown" );
	}

	close( internet_socket );
  pthread_join(receive, NULL);
	WSACleanup();

	return 0;
}
