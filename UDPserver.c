#ifdef _WIN32
	#define _WIN32_WINNT _WIN32_WINNT_WIN7
	#include <winsock2.h> //for all socket programming
	#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h> //for Timing
	void OSInit( void )
	{
		WSADATA wsaData;
		int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
		if( WSAError != 0 )
		{
			fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
			exit( -1 );
		}
	}
	void OSCleanup( void )
	{
		WSACleanup();
	}
	#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
	#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	#include <time.h> //for Timing
	int OSInit( void ) {}
	int OSCleanup( void ) {}
#endif

int initialization();
void execution( int internet_socket );
void cleanup( int internet_socket );

int main( int argc, char * argv[] )
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	execution( internet_socket );


	////////////
	//Clean up//
	////////////

	cleanup( internet_socket );

	OSCleanup();

	return 0;
}

int initialization()
{
	struct addrinfo internet_address_setup;
	struct addrinfo * internet_address_result;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo( NULL, "24042", &internet_address_setup, &internet_address_result );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 1 );
	}

	int internet_socket = -1;
	struct addrinfo * internet_address_result_iterator = internet_address_result;
	while( internet_address_result_iterator != NULL )
	{
		internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
		if( internet_socket == -1 )
		{
			perror( "socket" );
		}
		else
		{
			int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
			if( bind_return == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				break;
			}
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo( internet_address_result );

	if( internet_socket == -1 )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 2 );
	}

	return internet_socket;
}

void execution( int internet_socket )
{
	//////////////////////////////////ADDED////////////////////////////////////////
	FILE *RECEIVEDPACKETS;
	RECEIVEDPACKETS = fopen("received_Packets.csv", "w+");
	FILE *STATISCALDATA;
	STATISCALDATA = fopen("statistical_Data.csv", "w+");
	///////////////////////////////////END/////////////////////////////////////////

	int number_of_bytes_received = 0;
	char buffer[1000];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;

	//////////////////////////////////ADDED////////////////////////////////////////
	int packet_loss_Counter_TimeOut = 0;
	int packet_Loss_Counter_Timeout_Print = 0;
	double packet_Loss_Percentage = 0.0;

	int amount_Of_Receiving_Packets=0;
	char yes_Or_no = "n";
	int timeout = 0;

	printf("Would u like to add a custom time-out? [y/n]\n");
	printf("The standard value of the time-out is 0 seconds.\n", );
	scanf("%c", &yes_or_no);

	if(yes_Or_no == "y")
	{
		printf("Enter custom timeout time in seconds:");
		scanf("%d", &timeout);
		fprintf(STATISCALDATA, "Custom timeout = %d seconds\n",timeout);
		timeout = timeout*1000;
	}
	else if(yes_Or_no == "n")
	{
		fprintf(STATISCALDATA, "No changes were made to time-out time, it remains 0 seconds.\n", );
	}
	else
	{
		printf("It looks like u did not chose y or n... exiting now...\n");
		exit(-1);
	}


	printf("How many packets would u like to receive? (enter numeral value)");
	scanf("%d", &amount_Of_Receiving_Packets);

	clock_t begin = clock();

		if (setsockopt(internet_socket, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0)
		{
			perror("Error");
		}

	for (i = 1; i < amount_Of_Receiving_Packets; i++) //For loop for the amount of packets
	{
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length );
		if( number_of_bytes_received == -1 )
		{
			perror( "recvfrom" );
			++packet_loss_Counter_TimeOut
		}
		else
		{
			fprintf(STATISCALDATA, "There should be %d amount of packets by choice of the user with time-out time = %d\n",amount_Of_Receiving_Packets, timeout);
			buffer[number_of_bytes_received] = '\0';
			printf( "Received packet %d : %s\n", amount_Of_Receiving_Packets, buffer );
			fprintf(RECEIVEDPACKETS, "packet %d : %s\n", amount_Of_Receiving_Packets, buffer );

		}

	}
	///////////////////////////////////END/////////////////////////////////////////

	int number_of_bytes_send = 0;
	number_of_bytes_send = sendto( internet_socket, "Packet(s) received!", 16, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length );
	if( number_of_bytes_send == -1 )
	{
		perror( "sendto" );
	}
}

void cleanup( int internet_socket )
{
	close( internet_socket );
}
