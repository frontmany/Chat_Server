#include<iostream>

#include"server.h"


/*
struct Client {
	SOCKET sock;
	std::string name;
};

struct Message {
	std::string partnerName;
	std::string message; 

	std::string serialize() const {
		std::ostringstream oss;
		oss << partnerName << '\n' << message;
		return oss.str();
	}

	static Message deserialize(const std::string& str) {
		std::istringstream iss(str);
		Message msg;
		std::getline(iss, msg.partnerName);
		std::getline(iss, msg.message);
		return msg;
	}
};

void handleConnection(SOCKET acceptSocket, std::vector<Client>& clients) {
	char buffer[200];
	int byteCount;
	bool firsMessage = true;

	printf("Connection handler started for socket: %d\n", (int)acceptSocket);
	while (true) {
		byteCount = recv(acceptSocket, buffer, 200, 0);
		if (byteCount > 0) {
			std::string s(buffer);
			Message msg = Message::deserialize(s);
			std::cout << "Message Received: " << msg.message << std::endl;
			std::cout << "assumed partner name is: " << msg.partnerName << std::endl;


			if (firsMessage == true) {
				auto it = std::find_if(clients.begin(), clients.end(), [acceptSocket](const Client& client) {
					return client.sock == acceptSocket;
					});
				if (it != clients.end()) {
					it->name = msg.message;
					firsMessage = false;

					char resp[20];
					strcpy(resp, "You are authorized");
					send(acceptSocket, resp, strlen(resp), 0);
					continue;
				}
				else {
					std::cout << "Client not found in vector." << std::endl;
				}
			}
			else {
				// Отправка сообщения
				auto it = std::find_if(clients.begin(), clients.end(), [&clients, msg](const Client& client) {
					return msg.partnerName == client.name;
					});
				if (it != clients.end()) {
					send(it->sock, msg.message.c_str(), strlen(msg.message.c_str()), 0);
				}
				else {
					std::cout << "Client not found in vector." << std::endl;
					std::string errrMsg = msg.partnerName + " not found :(";
					send(acceptSocket, errrMsg.c_str(), strlen(errrMsg.c_str()), 0);
				}
			}
			
		}
		else if (byteCount == 0) {
			printf("Connection closed by peer.\n");
			break;
		}
		else {
			int error = WSAGetLastError();
			if (error == WSAECONNRESET) {
				printf("Сlient side connection shutdown. %d\n", error);
				break;
			}
			else {
				fprintf(stderr, "recv failed: %d\n", error);
				break;
			}
		}
	}
	closesocket(acceptSocket);
	printf("Connection handler exited for socket: %d\n", (int)acceptSocket);
}
*/

int main(int argc, char* argv[]) {
	/*
	WSADATA wsaData;
	int wsaError;
	WORD wVersionRequested = MAKEWORD(2, 2);
	wsaError = WSAStartup(wVersionRequested, &wsaData);
	if (wsaError != 0) {
		std::cout << "The Winsock dll not found!" << std::endl;
	}
	else {
		std::cout << "The Winsock dll found!" << std::endl;
		std::cout << "The status: " << wsaData.szSystemStatus << std::endl;
	}

	std::vector<SOCKET> sockets;

	int port = 54000;
	
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cout << "Error at \"socket()\": " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 0;
	}
	else {
		std::cout << "\"socket()\" is OK!" << std::endl;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	InetPton(AF_INET, "192.168.1.49", &service.sin_addr.s_addr);
	service.sin_port = htons(port);
	if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		std::cout << "\"bind()\" failed:" << WSAGetLastError() << std::endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}
	else {
		std::cout << "\"bind()\" is OK!" << std::endl;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "\"listen()\" Error listening on socket!" << WSAGetLastError() << std::endl;
	}
	else {
		std::cout << "\"listen()\" is ok, waiting for connections..." << std::endl;
	}



	//here
	std::vector<std::thread> threads;
	std::vector<Client> clients;
	while (true) {
		fd_set  readfds;
		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);

		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		int activity = select(0, &readfds, NULL, NULL, &timeout);

		if (activity == SOCKET_ERROR) {
			int error = WSAGetLastError();
			fprintf(stderr, "select failed: %d\n", error);
			break;
		}
		else if (activity > 0) {
			if (FD_ISSET(serverSocket, &readfds)) {
				SOCKET acceptSocket = accept(serverSocket, NULL, NULL);
				if (acceptSocket != INVALID_SOCKET) {
					printf("Connection accepted, socket number: %d \n", (int)acceptSocket);
					Client c;
					c.name = "";
					c.sock = acceptSocket;
					clients.emplace_back(c);
					threads.emplace_back(&handleConnection, acceptSocket, std::ref(clients));
					
				}
				else {
					int error = WSAGetLastError();
					fprintf(stderr, "accept failed: %d\n", error);
				}
			}
		}
		else {
			//printf("No connections pending.\n");
		}
	}
	
	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	closesocket(serverSocket);
	system("pause");
	WSACleanup();
	return 0;
	*/
	Server server;
	server.init("192.168.1.49", 54000);
	server.run();
}