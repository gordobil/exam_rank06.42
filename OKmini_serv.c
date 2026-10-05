#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/select.h>

int					_socket, _maxFd = 0, _clientCount = 0, _clients[FD_SETSIZE];
fd_set				_readSet;
struct sockaddr_in	_address;
bool				_active[FD_SETSIZE];
char				*_bufs[FD_SETSIZE];

void errorNExit(char* message){
	write(2, message, strlen(message));
	exit(1);
}

void sendStatusToClients(int clientSocket, char *message, int len){
	for (int fd = 0; fd <= _maxFd; fd++){
		if (_active[fd] && fd != clientSocket)
			send(fd, message, len, MSG_NOSIGNAL);
	}
}

void sendStatus(int clientSocket, int connexion){
	char	message[100] = {};

	if (connexion == 1)
		sprintf(message, "server: client %d just arrived\n", _clients[clientSocket]);
	else
		sprintf(message, "server: client %d just left\n", _clients[clientSocket]);
	sendStatusToClients(clientSocket, message, strlen(message));
}

void sendingMessage(int clientSocket, char *chunk){
	char	*buf = _bufs[clientSocket];
	size_t	old = buf ? strlen(buf) : 0;
	char	*tmp = realloc(buf, old + strlen(chunk) + 1);
	char	introBuf[100] = {};
	char	*start = tmp;
	char	*nl;

	if (!tmp)
		errorNExit("Fatal error\n");
	if (!buf)
		tmp[0] = '\0';
	strcat(tmp, chunk);

	sprintf(introBuf, "client %d: ", _clients[clientSocket]);

	while ((nl = strstr(start, "\n"))){
		sendStatusToClients(clientSocket, introBuf, strlen(introBuf));
		sendStatusToClients(clientSocket, start, nl - start + 1);
		start = nl + 1;
	}

	char	*rest = malloc(strlen(start) + 1);
	if (!rest)
		errorNExit("Fatal error\n");
	strcpy(rest, start);
	free(tmp);
	_bufs[clientSocket] = rest;
}

void disconnectClient(int clientSocket){
	sendStatus(clientSocket, 2);
	free(_bufs[clientSocket]);
	_bufs[clientSocket] = NULL;
	_active[clientSocket] = false;
	close(clientSocket);
}

void checkSocket(int clientSocket){
	char	clientBuf[4096] = {};
	int		recvLen = recv(clientSocket, clientBuf, sizeof(clientBuf) - 1, 0);

	if (recvLen > 0)
		sendingMessage(clientSocket, clientBuf);
	else
		disconnectClient(clientSocket);
}

void newClient(){
	struct sockaddr_in	clientAddress;
	socklen_t			clientLen = sizeof(clientAddress);

	int clientSocket = accept(_socket, (struct sockaddr*)&clientAddress, &clientLen);
	if (clientSocket > 0){
		if (clientSocket > _maxFd)
			_maxFd = clientSocket;
		_clients[clientSocket] = _clientCount++;
		_active[clientSocket] = true;
		sendStatus(clientSocket, 1);
	}
}

void serverLoop(){
	while (1){
		FD_ZERO(&_readSet);
		FD_SET(_socket, &_readSet);
		if (_socket > _maxFd)
			_maxFd = _socket;

		for (int fd = 0; fd <= _maxFd; fd++){
			if (_active[fd])
				FD_SET(fd, &_readSet);
		}

		if (select(_maxFd + 1, &_readSet, NULL, NULL, NULL) <= 0)
			continue ;

		for (int fd = 0; fd <= _maxFd; ++fd){
			if (!(FD_ISSET(fd, &_readSet)))
				continue ;
			if (fd == _socket)
				newClient();
			else
				checkSocket(fd);
		}
	}
}

int main(int argc, char** argv) {
	if (argc != 2)
		errorNExit("Wrong number of arguments\n");
	
	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
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
