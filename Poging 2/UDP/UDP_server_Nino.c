#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <unistd.h>

void print_ip_address( unsigned short family, struct sockaddr * ip )
{
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

void ai_print_ip_address( struct addrinfo * ip )
{
	print_ip_address( ip->ai_family, ip->ai_addr );
}

void ss_print_ip_address( struct sockaddr_storage * ip )
{
	print_ip_address( ip->ss_family, (struct sockaddr*) ip );
}

void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

void dissect(double* min, double* max, double* avg, char* buffer) {
	int count = 0;
	int j = 0;
	char tmp[1000] = "\0";
	double data = 0.0;

	for (int i = 0; count < 3; i++) {
		if (buffer[i] == ',') {
			++count;
		}
		if (count == 2 && buffer[i] != ',') {
			tmp[j] = buffer[i];
			++j;
		}
	}
	tmp[j] = '\0';

	data = strtod(tmp, NULL);
	printf("Extraced: %f\n", data);
	if (data < *min || *min == 0.0) {
		*min = data;
	}
	if (data > *max) {
		*max = data;
	}
	*avg += data;
}

int main( int argc, char * argv[] )
{
	WSADATA wsaData; //WSAData wsaData; //Could be different case
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 ) // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}

	char serverIP[45]= "\0";
	char serverPoort[5]= "\0";
	int amount = 0;
	int timeOut = 0;

	printf("Server IP: \n");
	scanf("%s", serverIP);
	fflush(stdin);

	printf("Server Poort: \n");
	scanf("%s", serverPoort);
	fflush(stdin);

	printf("Amout of packages: \n");
	scanf("%d", amount);
	fflush(stdin);

	printf("Time out: \n");
	scanf("%d", timeOut);
	fflush(stdin);

	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE; // use ANY address for IPv4 and IPv6

	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( serverIP, serverPoort, &internet_address_setup, &result_head);
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
			int return_code;
			return_code = bind( internet_socket, result_item->ai_addr, result_item->ai_addrlen );
			if( return_code == -1 )
			{
				close( internet_socket );
				perror( "bind" );
			}
			else
			{
				printf( "Bind to " );
				ai_print_ip_address( result_item );
				break; //stop running through the linked list
			}
		}
		result_item = result_item->ai_next; //take next in the linked list
	}
	if( result_item == NULL )
	{
		fprintf( stderr, "socket: no valid socket address found\n" );
		exit( 3 );
	}
	freeaddrinfo( result_head ); //free the linked list

	FILE *csvOUT = NULL;
	FILE *stats = NULL;

	printf("Creating CSV file...\n");
	csvOUT = fopen("PacketData.csv", "w");
	if (csvOUT == NULL)) {
		printf("PacketDate.csv not made\n");
		exit(4);
	}

	printf("Creating Stats file...\n");
	stats = fopen("Stats.csv", "w");
	if (stats == NULL)) {
		printf("stast.csv not made\n");
		exit(4);
	}

	fd_set fds ;
	int n ;
	struct timeval tv ;

	// Set up the file descriptor set.
	FD_ZERO(&fds) ;
	FD_SET(internet_socket, &fds) ;

	// Set up the struct timeval for the timeout.
	tv.tv_sec = timeOut;
	tv.tv_usec = 0 ;

	//data bijhouden
	double min = 0.0;
	double max = 0.0;
	double avg = 0.0;

	//receiving messages
	int number_of_bytes_received = 0;
	char buffer[1000];
	clock_t start;
	int i;

	struct sockaddr_storage client_ip_address;
	socklen_t client_ip_address_length = sizeof client_ip_address;

	printf("Luisteren...\n");
	for (i = 0; i < amount; i++)
	{
		strcpy(buffer, "\0");

		// Wait until timeout or data received.
		n = select ( internet_socket, &fds, NULL, NULL, &tv ) ;
		if (n == 0)
		{
		  printf("Timeout..\n");
		 	exit(5);
		}
		else if( n == -1 )
		{
		  printf("Error..\n");
		  exit(6);
		}

		number_of_bytes_received = 0;
		number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_ip_address, &client_ip_address_length );
		if( number_of_bytes_received == -1 )
		{
			printf( "errno = %d\n", WSAGetLastError() ); //https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
			perror( "recvfrom" );
		}

		if(i == 0)
			{
				start = clock();
			}
		buffer[number_of_bytes_received] = '\0';
		printf( "Got %s from ", buffer );
		ss_print_ip_address( &client_ip_address );

		printf("%s\n", buffer);
		remove_spaces(buffer);


	}

	close( internet_socket );

	WSACleanup();

	return 0;
}
