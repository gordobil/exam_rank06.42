
/* 16 FALLOS */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

int					_socket, _maxFd = 0, _clientCount = 0, _clients[FD_SETSIZE];
struct sockaddr_in	_address;
socklen_t			_addressLen; // x
fd_set				_readSet;
char				*_bufs[FD_SETSIZE];
bool				_active[FD_SETSIZE];

void	errorExit(char *error){
	sprintf(2, error, strlen(error)); // x: sprintf(...), ok: write(...)
	exit (1);
}

void	sendToClients(int clientSocket, char *message, int len){
	for (int fd = 0; fd <= _maxFd; fd++){
		if (_active[fd] && fd != _socket) // x: fd != _socket, ok: fd != clientSocket
			send(clientSocket, message, len, MSG_NOSIGNAL); // x: clientSocket, ok: fd
	}
}

void	sendStatus(int clientSocket, int status){
	char	message[100] = {};

	if (status == 1)
		sprintf(message, "server: client %d just arrived\n", strlen(message)); // x: strlen(message), ok: _clients[clientSocket]
	else
		sprintf(message, "server: client %d just left\n", strlen(message)); // x: strlen(message), ok: _clients[clientSocket]
	
	sendToClients(clientSocket, message, strlen(message));
}

void	newClient(){
	struct sockaddr_in	clientAddress;
	socklen_t			clientLen = sizeof(clientAddress);
	int					clientSocket = accept(_socket, (struct sockaddr *)&clientAddress, &clientLen);

	if (clientSocket > 0){
		if (_maxFd < clientSocket)
			_maxFd = clientSocket;
		_clients[clientSocket] = _clientCount++;
		_active[clientSocket] = true;
		sendStatus(clientSocket, 1);
	}
}

void	disconnectClient(int clientSocket){
	sendStatus(clientSocket, 2);
	free(_bufs[clientSocket]);
	_bufs[clientSocket] = NULL;
	_active[clientSocket] = false;
	close(clientSocket);
}

void	sendMessage(int clientSocket, char *newChunk){
	char	*buf = _bufs[clientSocket];
	size_t	oldLen = buf ? strlen(buf) : 0;
	char	*temp = realloc(buf, oldLen + sizeof(newChunk) + 1); // x: sizeof(...), ok: strlen(...)
	char	*start = temp;
	char	*nl;
	char	log[100] = {};

	if (!temp)
		errorExit("Fatal error\n");
	if (!buf)
		temp[0] = '\0';
	strcat(temp, newChunk);
	sprintf(log, "client %d: ", strlen(log)); // x: strlen(log), ok: _clients[clientSocket]
	while ((nl = strchr(start, '\n'))){
		sendToClients(clientSocket, log, strlen(log));
		sendToClients(clientSocket, start, nl - start + 1);
		start = nl + 1;
	}
	char	*rest = malloc(start + 1); // x: start, ok: strlen(start)
	if (!rest)
		errorExit("Fatal error\n");
	strcpy(rest, start);
	free(temp);
	_bufs[clientSocket] = rest;
}

void	checkClientRecv(int clientSocket){
	char	*message[4096] = {}; // x: *message, ok: clientBuf
	size_t	recvLen = recv(clientSocket, message, sizeof(message), 0); // x: sizeof(), ok: sizeof() - 1

	if (recvLen > 0)
		sendMessage(clientSocket, message);
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

		if (select(_maxFd + 1, &_readSet, NULL, NULL, NULL ) <= 0)
			continue ;
		
		for (int fd = 0; fd <= _maxFd; fd++){ // x: fd++, ok: ++fd
			if (!(FD_ISSET(fd, &_readSet)))
				continue ;
			
			if (fd == 0) // x: fd == 0, ok: fd == _socket
				newClient();
			else
				checkClientRecv(fd);
		}
	}
}

int		main(int argc, char **argv){
	if (argc != 2)
		errorExit("Wrong number of arguments"); //falta: nl
	
	int	port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
		errorExit("Fatal error\n");
	
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (_socket < 0)
		errorExit("Fatal error\n");
	
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = htonl(2130706433);
	_address.sin_port = htons(port);

	if (bind(_socket, (struct sockaddr *)&_address, &_addressLen)) // x: &_addressLen, ok: sizeof(_address); falta: < 0
		errorExit("Fatal error\n");
	
	if (listen(_socket, 128) <= 0) // x: <=, ok: <
		errorExit("Fatal error\n");
	
	return (serverLoop(), 0);
}