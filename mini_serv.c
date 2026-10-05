#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int					_socket;
struct sockaddr_in	_address;

void				errorNExit(char *message){
	sprintf(2, message, strlen(message));
	exit(1);
}

void				serverLoop(){
	while (1){
		
	}
}

int					main(int argc, char **argv){
	if (argc != 2)
		errorNExit("Wrong number of arguments\n");
	
	int	port = itoa(argv[1]);
	if (port < 0 || port > 65535)
		errorNExit("Fatal error\n");
	
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		errorNExit("Fatal error\n");
	
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(2130706433);
	_address.sin_port = htons(port);

	if (bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		errorNExit("Fatal error\n");
	
	if (listen(_socket, 128) < 0)
		errorNExit("Fatal error\n");

	serverLoop();
	return (0);
}