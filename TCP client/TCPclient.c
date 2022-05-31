// gcc 7_TCP_client.c -lws2_32 -o 7_TCP_client.exe
// _WIN32_WINNT version constants --> https://stackoverflow.com/questions/15370033/how-to-use-inet-pton-with-the-mingw-compiler

#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

int stop_recv_thread=0;

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
void *received_data(void *server_socket){
	int server_socket_2 = (intptr_t) server_socket;
	char buffer[420]="\0";
	int number_of_bytes_received=0;

	while(stop_recv_thread ==0){
		recv(server_socket_2, buffer , sizeof(buffer), 0);
	if (number_of_bytes_received == -1)
	{
		perror("recv");
	}
	printf(" - %s\n",buffer);
	buffer[number_of_bytes_received] = '\0';
}
	pthread_exit(NULL);
	return NULL;
}


int main( int argc, char * argv[] )
{
	WSADATA wsaData;
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 )
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}

	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;

	char ip_address_server[45] = "\n";
	char poort_nummer_server[5];
	printf("Geef het ip van de server in:\n");
	scanf("%s", ip_address_server);
	fflush(stdin);

	printf("Geef het Poort nummer van de server in:\n");
	scanf("%s", poort_nummer_server);
	fflush(stdin);
	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( ip_address_server, poort_nummer_server, &internet_address_setup, &result_head );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 2 );
	}

	//struct sockaddr * internet_address;
	//size_t internet_address_length;

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

	int number_of_bytes_send = 0;
	char user_message[500]= "\0";


	if( number_of_bytes_send == -1 )
	{
		printf( "errno = %d\n", WSAGetLastError() );
		perror( "send" );
	}

		pthread_t receive_Thread;
		if (pthread_create(&receive_Thread, NULL, received_data, (void *) (intptr_t) internet_socket ) !=0){
			printf("ERROR\n");

		}

	while (1)  {
		printf("-");
		scanf("%s", user_message);
		fflush(stdin);
		if (strcmp(user_message, "/exit") == 0)
		{
			break;
		}
		number_of_bytes_send = send( internet_socket, user_message, strlen(user_message), 0 );
	}

	int shutdown_return;
	stop_recv_thread = 1;
	pthread_join(receive_Thread,NULL);
	shutdown_return = shutdown( internet_socket, SD_SEND );
	if( shutdown_return ==1){
		printf("errno=%d\n", WSAGetLastError() );
		perror("shutdown");
	}
	{
		printf( "errno = %d\n", WSAGetLastError() );
		perror( "shutdown" );
	}

	close( internet_socket );

	WSACleanup();

	return 0;
}
