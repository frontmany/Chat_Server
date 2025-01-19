#include"server.h"

Server::Server()
	: m_ipAddress(""), m_port(-1), m_listener_socket(INVALID_SOCKET) {
}

void Server::init(std::string ipAddress, int port) {
	m_ipAddress = ipAddress;
	m_port = port;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	int wsaError = WSAStartup(wVersionRequested, &wsaData);
	if (wsaError != 0) {
		throw std::runtime_error("The Winsock dll not found!");
	}

	m_listener_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_listener_socket == INVALID_SOCKET) {
		throw std::runtime_error("Failed to initialize listener socket!");
		WSACleanup();
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	InetPton(AF_INET, m_ipAddress.c_str(), &service.sin_addr.s_addr);
	service.sin_port = htons(m_port);
	if (bind(m_listener_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		throw std::runtime_error("Failed to bind listener socket!");
		closesocket(m_listener_socket);
		WSACleanup();
	}

	if (listen(m_listener_socket, SOMAXCONN) == SOCKET_ERROR) {
		throw std::runtime_error("Failed to start listening on listener socket!");
	}
}

void Server::run() {
	while (true) {
		fd_set  readfds;
		FD_ZERO(&readfds);
		FD_SET(m_listener_socket, &readfds);

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
			if (FD_ISSET(m_listener_socket, &readfds)) {
				SOCKET acceptSocket = accept(m_listener_socket, NULL, NULL);
				if (acceptSocket != INVALID_SOCKET) {
					printf("Connection accepted, socket number: %d \n", (int)acceptSocket);
					m_vec_threads.emplace_back(&Server::onReceiving, this, acceptSocket);
				}
				else {
					int error = WSAGetLastError();
					throw std::runtime_error("Failed to accept connection!");
				}
			}
		}
	}
}

void Server::authorizeClient(int acceptSocket) {
	bool isAuthorized = false;
	while (!isAuthorized) {
		char buffer[200];
		int byteCount = recv(acceptSocket, buffer, 200, 0);
		if (byteCount > 0) {
			std::string bufferString(buffer, byteCount);
			req::SenderData data = req::SenderData::deserialize(bufferString);

			if (data.needsRegistration) {
				std::lock_guard<std::mutex> lock(m_mtx);
				m_vec_users.emplace_back(data.login, data.password, data.name); // add user to db
				sendStatus(acceptSocket, ServerResponse::REGISTRATION_SUCCESS);
				m_vec_online_users.emplace_back(data.login, data.password, data.name, true, acceptSocket);
				isAuthorized = true;
			}
			else {
				std::lock_guard<std::mutex> lock(m_mtx);
				auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&data](const User& user) {
					return ((user.getLogin() == data.login) && (user.getPassword() == data.password));
					});

				if (it != m_vec_users.end()) {
					sendStatus(acceptSocket, ServerResponse::AUTHORIZATION_SUCCESS);
					it->setStatus(true); // means online
					it->setSocketOnServer(acceptSocket);
					m_vec_online_users.emplace_back(it->getLogin(), it->getPassword(),
						it->getName(), it->getStatus(), it->getSocketOnServer());
					isAuthorized = true;
				}

				else {
					sendStatus(acceptSocket, ServerResponse::AUTHORIZATION_FAIL);
				}
			}
		}
		else {
			std::cout << "receiving error"; //remove later
		}
	}
}


void Server::onReceiving(int acceptSocket) {
	authorizeClient(acceptSocket);
	char buffer[1500];
	int byteCount;

	while (true) {
		byteCount = recv(acceptSocket, buffer, 200, 0);
		if (byteCount > 0) {
			std::string s(buffer, byteCount);

			if (s.substr(0, 13) == "GET_USER_INFO") {
				auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&s](const User& user) {
					return s.substr(14) == user.getLogin();
					});

				if (it == m_vec_users.end()) {
					sendStatus(acceptSocket, ServerResponse::USER_INFO_NOT_FOUND);
					continue;

				}
				else {
					req::Packet pack;
					req::SenderData dat("");
					dat.login = it->getLogin();
					dat.name = it->getName();
					dat.password = it->getPassword();
					pack.sender = dat;
					std::string s = "USER_INFO_FOUND" + pack.serialize();
					send(acceptSocket, s.c_str(), s.length(), 0);
					continue;
				}
			}

			req::Packet packet = req::Packet::deserialize(s);

			
			if (packet.isNewChat) {
				auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](const User& user) {
					return packet.receiver.login == user.getLogin();
					});

				if (it == m_vec_users.end()) {
					sendStatus(acceptSocket, ServerResponse::CHAT_CREATE_FAIL);

				}
				else {
					sendStatus(acceptSocket, ServerResponse::CHAT_CREATE_SUCCESS);
				}
			}
			else {
				auto it = std::find_if(m_vec_online_users.begin(), m_vec_online_users.end(), [&packet](const User& user) {
					return packet.receiver.login == user.getLogin();
					});
				if (it == m_vec_online_users.end()) {

					auto it = m_map_messages_to_send.find(packet.receiver.login);
					if (it == m_map_messages_to_send.end()) {
						std::vector<std::string> messagesVec;
						messagesVec.emplace_back(packet.msg);
						m_map_messages_to_send[packet.receiver.login] = messagesVec;
					}
					else {
						std::vector<std::string>& messagesVec = m_map_messages_to_send[packet.receiver.login];
						messagesVec.emplace_back(packet.msg);
					}
				}
				else {
					sendPacket(acceptSocket, packet);
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
				printf("Ñlient side connection shutdown. %d\n", error);
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


void Server::sendBool(SOCKET acceptSocket, bool value) {
	const char* resp = value ? "true" : "false";
	send(acceptSocket, resp, strlen(resp), 0);
}

void Server::sendPacket(SOCKET acceptSocket, req::Packet packet) {
	std::string s = packet.serialize();
	send(acceptSocket, s.c_str(), strlen(s.c_str()), 0);
}

void Server::sendStatus(SOCKET acceptSocket, ServerResponse response) {
	const char* responseStr = nullptr;

	if (response == ServerResponse::AUTHORIZATION_SUCCESS) {
		responseStr = "AUTHORIZATION_SUCCESS";
	}
	else if (response == ServerResponse::REGISTRATION_SUCCESS) {
		responseStr = "REGISTRATION_SUCCESS";
	}
	else if (response == ServerResponse::AUTHORIZATION_FAIL) {
		responseStr = "AUTHORIZATION_FAIL";
	}
	else if (response == ServerResponse::REGISTRATION_FAIL) {
		responseStr = "REGISTRATION_FAIL";
	}
	else if (response == ServerResponse::CHAT_CREATE_SUCCESS) {
		responseStr = "CHAT_CREATE_SUCCESS";
	}
	else if (response == ServerResponse::CHAT_CREATE_FAIL) {
		responseStr = "CHAT_CREATE_FAIL";
	}
	else {
		responseStr = "";
	}
	send(acceptSocket, responseStr, strlen(responseStr), 0);
}