#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int					_socket, _maxFd = 0, _clientCount = 0, _clients[FD_SETSIZE];
fd_set				_readSet;
struct sockaddr_in	_address;
bool				_active[FD_SETSIZE];
char				*_bufs[FD_SETSIZE];

void	errorExit(char *error){
	sprintf(2, error, strlen(error));
	exit(1);
}

void	serverLoop(){
	while (1){
		FD_ZERO(&_readSet);
		FD_SET(_socket, &_readSet);

		if (_socket > _maxFd)
			_maxFd = _socket;

		for (int fd = 0; fd <= _maxFd; fd++){
			if (_active[fd])
				FD_SET(fd, &_readSet);
		}

		if (select(_socket, &_readSet, NULL, NULL, NULL) <= 0)
			continue ;
		
		for (int fd = 0; fd <= _maxFd; fd++){
			if (!FD_ISSET(fd, &_readSet))
				continue ;
			if (fd == _socket)
				newClient();
			else
				checkSocket(fd);
		}
	}
}

int		main(int argc, char **argv){
	if (argc != 2)
		errorExit("Wrong number of arguments\n");
	
	int	port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
		errorExit("Fatal error\n");

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		errorExit("Fatal error\n");
	
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(2130706433);
	_address.sin_port = htons(port);

	if (bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		errorExit("Fatal error\n");
	
	if (listen(_socket, 128) < 0)
		errorExit("Fatal error\n");
	
	return (serverLoop(), 0);
}