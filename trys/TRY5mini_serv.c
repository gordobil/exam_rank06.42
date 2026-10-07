
/* 2 FALLOS */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int					_socket, _maxFd = 0, _clientCount = 0, _clients[FD_SETSIZE];
fd_set				_readSet;
struct sockaddr_in	_address;
bool				_active[FD_SETSIZE];
char				*_bufs[FD_SETSIZE];

void	errorExit(char *error){
	write(2, error, strlen(error));
	exit(1);
}

void	sendToClients(int clientSocket, char *message, int len){
	for (int fd = 0; fd <= _maxFd; fd++){
		if (_active[fd] && fd != clientSocket)
			send(fd, message, len, MSG_NOSIGNAL);
	}
}

void	sendStatus(int clientSocket, int status){
	char	message[100] = {};

	if (status == 1)
		sprintf(message, "server: client %d just arrived\n", _clients[clientSocket]);
	else
		sprintf(message, "server: client %d just left\n", _clients[clientSocket]);
	
	sendToClients(clientSocket, message, strlen(message));
}

void	newClient(){
	struct sockaddr_in	clientAddress;
	socklen_t			clientLen = sizeof(clientAddress);
	int					clientSocket = accept(_socket, (struct sockaddr *)&clientAddress, &clientLen);

	if (clientSocket > 0){ // x: >, ok: >=
		if (_maxFd < clientSocket)
			_maxFd = clientSocket;
		_clients[clientSocket] = _clientCount++;
		_active[clientSocket] = true;
		sendStatus(clientSocket, 1);
	}
}

void	sendMessage(int clientSocket, char *newChunk){
	char	*buf = _bufs[clientSocket];
	int		oldLen = buf ? strlen(buf) : 0;
	char	*temp = realloc(buf, oldLen + strlen(newChunk) + 1);
	char	*start = temp;
	char	*nl;
	char	log[100] = {};

	if (!temp)
		errorExit("Fatal error\n");
	if (!buf)
		temp[0] = '\0';
	
	strcat(temp, newChunk);
	sprintf(log, "client %d: ", _clients[clientSocket]);
	while ((nl = strstr(start, "\n"))){
		sendToClients(clientSocket, log, strlen(log));
		sendToClients(clientSocket, start, nl - start + 1);
		start = nl + 1;
	}
	char	*rest = malloc(strlen(start) + 1);
	if (!rest)
		errorExit("Fatal error\n");
	
	strcpy(rest, start);
	free(temp);
	_bufs[clientSocket] = rest;
}

void	disconnectClient(int clientSocket){
	free(_bufs[clientSocket]);
	_bufs[clientSocket] = NULL;
	close(clientSocket);
	_active[clientSocket] = false;
	sendStatus(clientSocket, 2);
}

void	checkClientRecv(int clientSocket){
	char	clientBuf[4096] = {};
	int		recvLen = recv(clientSocket, clientBuf, strlen(clientBuf) - 1, 0);

	if (recvLen > 0)
		sendMessage(clientSocket, clientBuf);
	else
		disconnectClient(clientSocket);
}

void	serverLoop(){
	while (1){
		FD_ZERO(&_readSet);
		FD_SET(_socket, &_readSet);

		if (_maxFd < _socket)
			_maxFd = _socket;
		
		for (int fd = 0; fd <= _maxFd; fd++){
			if (_active[fd])
				FD_SET(fd, &_readSet);
		}

		if (select(_maxFd + 1, &_readSet, NULL, NULL, NULL) < 0) // x: <, ok: <=
			continue ;
		
		for (int fd = 0; fd <= _maxFd; ++fd){
			if (!(FD_ISSET(fd, &_readSet)))
				continue ;
			
			if (fd == _socket)
				newClient();
			else
				checkClientRecv(fd);
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