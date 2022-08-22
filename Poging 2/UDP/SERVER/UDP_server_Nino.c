#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

//PRINTS CONNECTION INFO
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

void ss_print_ip_address( struct sockaddr_storage * ip ) {
	print_ip_address( ip->ss_family, (struct sockaddr*) ip );
}
//PRINTS CONNECTION INFO

//REMOVES SPACES
//https://stackoverflow.com/questions/1726302/remove-spaces-from-a-string-in-c
void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while ((*s++ = *d++));
}
//REMOVES SPACES

//DISSECT PACKET DATA
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
	printf("[+] Extraced %f\n", data);
	if (data < *min || *min == 0.0) {
		*min = data;
	}
	if (data > *max) {
		*max = data;
	}
	*avg += data;
}
//DISSECT PACKET DATA

int main( int argc, char * argv[] ) {
//START SOCKET API
	WSADATA wsaData; //WSAData wsaData; //Could be different case
	if( WSAStartup( MAKEWORD(2,0), &wsaData ) != 0 ) // MAKEWORD(1,1) for Winsock 1.1, MAKEWORD(2,0) for Winsock 2.0:
	{
		fprintf( stderr, "WSAStartup failed.\n" );
		exit( 1 );
	}
//START SOCKET API

//ASK USER FOR SERVER IP, PORT, AMOUNT
  char server_ip[45] = "\0";
  char server_port[5] = "\0";
  int amount = 0;
  int timeout = 0;

  printf("\nSet Server IP:\n");
  scanf("%s", server_ip);
  fflush(stdin);

  printf("\nSet UDP Server PORT:\n");
  scanf("%s", server_port);
  fflush(stdin);

  printf("\nAmount Of Packets:\n");
	scanf("%d", &amount);
	fflush(stdin);

  printf("\nTimeout (in seconds):\n");
	scanf("%d", &timeout);
	fflush(stdin);
//ASK USER FOR SERVER IP, PORT, AMOUNT

//SOCKET SETUP
	struct addrinfo internet_address_setup, *result_head, *result_item;
	memset( &internet_address_setup, 0, sizeof internet_address_setup );
	internet_address_setup.ai_family = AF_INET; // AF_INET or AF_INET6 to force version AF_UNSPEC
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE; // use ANY address for IPv4 and IPv6

	int getaddrinfo_return;
	getaddrinfo_return = getaddrinfo( server_ip, server_port, &internet_address_setup, &result_head );
	if( getaddrinfo_return != 0 )
	{
		fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
		exit( 2 );
	}
//SOCKET SETUP

//CREATE SOCKET (internet_address_setup)
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
				printf( "\nBind to " );
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
//CREATE SOCKET (internet_address_setup)

//FILE HANDLING
  FILE *outFile = NULL;
	FILE *statsFile = NULL;

  printf("[+] Creating output.csv...\n");
  outFile = fopen("packet_data.csv", "w");
  if (outFile == NULL) {
    printf("\n[-] Could not create packet_data.csv!\n");
    exit(4);
  }

	printf("[+] Creating statistics.txt...\n");
  statsFile = fopen("statistics.txt", "w");
  if (statsFile == NULL) {
    printf("\n[-] Could not create statistics.txt!\n");
    exit(4);
  }
//FILE HANDLING

//TIMEOUT SETUP
//https://stackoverflow.com/questions/1824465/set-timeout-for-winsock-recvfrom
	fd_set fds;
	int n;
	struct timeval tv;

// Set up the file descriptor set.
	FD_ZERO(&fds) ;
	FD_SET(internet_socket, &fds) ;

// Set up the struct timeval for the timeout.
	tv.tv_sec = timeout; //tv.tv_sec SECONDS
	tv.tv_usec = 0;
//TIMEOUT SETUP

//PACKET DATA
	double min = 0.0;
	double max = 0.0;
	double avg = 0.0;
//PACKET DATA

//RECEIVE MSG
	int number_of_bytes_received = 0;
	char buffer[1000] = "\0";
	clock_t begin_time;
	int i;

  struct sockaddr_storage client_ip_address;
	socklen_t client_ip_address_length = sizeof client_ip_address;

  printf("Listening...\n");
  for (i = 0; i < amount; i++) {
    strcpy(buffer, "\0");

		//TIMEOUT
		n = select ( internet_socket, &fds, NULL, NULL, &tv );
		if ( n == 0) {
			printf("Timeout\n");
			printf("RECV: %d | EXPE: %d | LOSS: %d%%\n", i, amount, abs(((i - amount)/amount)*100));
			printf("MIN: %f | MAX: %f | AVG: %f\n", min, max, (avg/i));
			exit(5);
		}
		else if( n == -1 ) {
			printf("Error in timeout!\n");
			exit(6);
		}
		//TIMEOUT

    number_of_bytes_received = 0;
    number_of_bytes_received = recvfrom( internet_socket, buffer, ( sizeof buffer ) - 1, 0, (struct sockaddr *) &client_ip_address, &client_ip_address_length );

  	if( number_of_bytes_received == -1 ) {
  		printf( "errno = %d\n", WSAGetLastError() ); //https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
  		perror( "recvfrom" );
  	}

		if (i == 0) {
			begin_time = clock();
		}

  	buffer[number_of_bytes_received] = '\0';

    printf("\nReceiving from ");
  	ss_print_ip_address( &client_ip_address );
    printf("%s\n", buffer);

    printf("Writing to output.csv...\n");
		remove_spaces(buffer);
		dissect(&min, &max, &avg, buffer);
    fwrite(&buffer, strlen(buffer), 1, outFile);
    fwrite("\n", sizeof(char), 1, outFile);

    printf("Packet %d/%d\n\n", i+1, amount);
  }
//RECEIVE MSG

//ELAPSED TIME
	clock_t end_time = clock();
	float elapsed_time = (double)(end_time - begin_time) / CLOCKS_PER_SEC;
//ELAPSED TIME

//WRITE STATSFILE
	char tmp[100] = "\0";

	printf("\n");

	sprintf(tmp, "Elapsed Time: %.2f sec\n", elapsed_time);
	printf("%s", tmp);
	fwrite(&tmp, strlen(tmp), 1, statsFile);

	sprintf(tmp, "RECV: %d | EXPE: %d | LOSS: %d%%\n", i, amount, abs(((i - amount)/amount)*100));
	printf("%s", tmp);
	fwrite(&tmp, strlen(tmp), 1, statsFile);

	sprintf(tmp, "MIN: %f | MAX: %f | AVG: %f\n", min, max, (avg/i));
	printf("%s", tmp);
	fwrite(&tmp, strlen(tmp), 1, statsFile);
//WRITE STATSFILE

//CLOSE CONNECTION & CLEANUP
  fclose(outFile);
	fclose(statsFile);
	close( internet_socket );
	WSACleanup();
//CLOSE CONNECTION & CLEANUP

	return 0;
}
