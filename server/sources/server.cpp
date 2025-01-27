#include"server.h"

Server::Server()
	: m_ipAddress(""), m_port(-1), m_listener_socket(INVALID_SOCKET) {
}

void Server::init(const std::string& ipAddress, int port) {
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

void Server::onReceiving(SOCKET acceptSocket) {
	bool isReceivingNextPacketSize = true;
	int bufferSize = 40;
	while (true) {
		char* buffer = new char[bufferSize];
		int byteCount = recv(acceptSocket, buffer, bufferSize, 0);

		if (byteCount > 0 && isReceivingNextPacketSize) {
			std::string bufferStr(buffer, byteCount);
			auto pair = rcv::parseQuery(bufferStr);
			bufferSize = std::stoi(pair.second);
			isReceivingNextPacketSize = false;
			continue;
		}

		if (byteCount > 0 && !isReceivingNextPacketSize) {
			std::string bufferStr(buffer, bufferSize);
			auto pair = rcv::parseQuery(bufferStr);

			if (pair.first == Query::AUTHORIZATION) {
				rcv::AuthorizationPacket packet = rcv::AuthorizationPacket::deserialize(pair.second);
				authorizeUser(acceptSocket, packet);
			}
			else if (pair.first == Query::REGISTRATION) {
				rcv::RegistrationPacket packet = rcv::RegistrationPacket::deserialize(pair.second);
				registerUser(acceptSocket, packet);
			}
			else if (pair.first == Query::CREATE_CHAT) {
				rcv::CreateChatPacket packet = rcv::CreateChatPacket::deserialize(pair.second);
				createChat(acceptSocket, packet);
			}
			else if (pair.first == Query::GET_USER_INFO) {
				rcv::GetUserInfoPacket packet = rcv::GetUserInfoPacket::deserialize(pair.second);
				findUserInfo(acceptSocket, packet);
			}
			else if (pair.first == Query::UPDATE_USER_INFO) {
				rcv::UpdateUserInfoPacket packet = rcv::UpdateUserInfoPacket::deserialize(pair.second);
				updateUserInfo(acceptSocket, packet);
			}
			else if (pair.first == Query::MESSAGE) {
				rpl::Message message = rpl::Message::deserialize(pair.second);
				sendMessage(acceptSocket, message);
			}
			else if (pair.first == Query::GET_ALL_FRIENDS_STATES) {
				rcv::GetFriendsStatusesPacket packet = rcv::GetFriendsStatusesPacket::deserialize(pair.second);
				findUsersStatuses(acceptSocket, packet);
			}

			isReceivingNextPacketSize = true;
			bufferSize = 40;
			delete buffer;
		}

		else {
			int error = WSAGetLastError();
			std::lock_guard<std::mutex> lock(m_mtx);

			auto it_me = std::find_if(m_vec_users.begin(), m_vec_users.end(),
				[acceptSocket](const User& user) {
					return user.getSocketOnServer() == acceptSocket;
				});
			it_me->setIsOnline(false);
			it_me->setLastSeenToNow();
			it_me->setSocketOnServer(-1);

			isReceivingNextPacketSize = true;
			delete buffer;

			if (error == WSAECONNRESET) {
				printf("Client side connection shutdown.\n", error);
				sendStatusToFriends(it_me, false);
				return;
				/*snd::FriendStatePacket packet;
				packet.setIsOnline(false);
				packet.setLastSeen(it_me->getLastSeen());

				for (auto friendLogin : it_me->getUserFriendsVec()) {
					auto itFriend = std::find_if(m_vec_users.begin(), m_vec_users.end(),
						[&friendLogin](const User& user) {
							return user.getLogin() == friendLogin;
						});
					if (itFriend->getIsOnline()) {
						sendPacket(itFriend->getSocketOnServer(), packet);
					}
				}

				break;
			}
			else {
				fprintf(stderr, "recv failed: %d\n", error);
				break;
			}*/
			}
		}


	}
}

void Server::findUsersStatuses(SOCKET acceptSocket, rcv::GetFriendsStatusesPacket& packet) {
	std::vector<std::string>&  usersLoginsVec = packet.getFriendsLoginsVec();

	snd::FriendsStatusesPacket pack;
	for (auto login : usersLoginsVec) {
		auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&login](User& user) {
			return user.getLogin() == login;
			});
		pack.getVecStatuses().push_back(std::make_pair(login, it->getLastSeen()));
	}
	sendPacket(acceptSocket, pack);
}

void Server::updateUserInfo(SOCKET acceptSocket, rcv::UpdateUserInfoPacket& packet) {
	std::lock_guard<std::mutex> lock(m_mtx);
	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
		return user.getLogin() == packet.getLogin();
		});

	if (it != m_vec_users.end()) {
		it->setLogin(packet.getLogin());
		it->setName(packet.getName());
		it->setPassword(packet.getPassword());
		it->setPhoto(packet.getPhoto());

		snd::StatusPacket pack;
		pack.setStatus(Responce::USER_INFO_UPDATED);
		sendPacket(acceptSocket, pack);
	}
	else {
		snd::StatusPacket pack;
		pack.setStatus(Responce::USER_INFO_NOT_UPDATED);
		sendPacket(acceptSocket, pack);
	}
}

void Server::findUserInfo(SOCKET acceptSocket, rcv::GetUserInfoPacket& packet) {
	std::lock_guard<std::mutex> lock(m_mtx);
	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
		return user.getLogin() == packet.getLogin();
		});

	if (it != m_vec_users.end()) {
		rpl::UserInfoPacket pack;
		pack.setLogin(it->getLogin());
		pack.setName(it->getName());
		pack.setIsHasPhoto(it->getIsHasPhoto());
		pack.setIsOnline(it->getIsOnline());
		pack.setLastSeen(it->getLastSeen());
		pack.setPhoto(it->getPhoto());
		sendPacket(acceptSocket, pack);


	}
	else {
		snd::StatusPacket pack;
		pack.setStatus(Responce::USER_INFO_NOT_FOUND);
		sendPacket(acceptSocket, pack);
	}
}

void Server::createChat(SOCKET acceptSocket, rcv::CreateChatPacket& packet) {
	if (packet.getSenderLogin() == packet.getReceiverLogin()) {
		snd::StatusPacket pack;
		pack.setStatus(Responce::CHAT_CREATE_FAIL);
		sendPacket(acceptSocket, pack);
		return;
	}

	std::lock_guard<std::mutex> lock(m_mtx);
	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
		return packet.getReceiverLogin() == user.getLogin();
		});

	if (it == m_vec_users.end()) {
		snd::StatusPacket pack;
		pack.setStatus(Responce::CHAT_CREATE_FAIL);
		sendPacket(acceptSocket, pack);

	}
	else {
		auto it_me = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
			return packet.getSenderLogin() == user.getLogin();
			});

		auto it_friend = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
			return packet.getReceiverLogin() == user.getLogin();
			});

		std::vector<std::string>& friendsVecSender = it_me->getUserFriendsVec();
		std::vector<std::string>& friendsVecReceiver = it_friend->getUserFriendsVec();
		friendsVecSender.push_back(packet.getReceiverLogin());
		friendsVecReceiver.push_back(packet.getSenderLogin());

		snd::ChatSuccessPacket pack;
		rpl::UserInfoPacket userPack;
		userPack.setIsHasPhoto(it->getIsHasPhoto());
		userPack.setIsOnline(it->getIsOnline());
		userPack.setLastSeen(it->getLastSeen());
		userPack.setLogin(it->getLogin());
		userPack.setName(it->getName());
		userPack.setPhoto(it->getPhoto());

		pack.setUserInfoPacket(userPack);
		sendPacket(acceptSocket, pack);
	}
}

void Server::registerUser(SOCKET acceptSocket, rcv::RegistrationPacket& packet) {
	std::lock_guard<std::mutex> lock(m_mtx);
	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
		return user.getLogin() == packet.getLogin();
		});

	if (it == m_vec_users.end()) {
		User user(packet.getLogin(), packet.getPassword(), packet.getName(), acceptSocket);
		user.setSocketOnServer(acceptSocket);
		user.setIsOnline(true);
		user.setLastSeen("online");

		m_vec_users.push_back(user); // add user to db
		snd::StatusPacket pack;
		pack.setStatus(Responce::REGISTRATION_SUCCESS);
		sendPacket(acceptSocket, pack);
	}
	else{
		snd::StatusPacket pack;
		pack.setStatus(Responce::REGISTRATION_FAIL);
		sendPacket(acceptSocket, pack);
	}
}

void Server::authorizeUser(SOCKET acceptSocket, rcv::AuthorizationPacket& packet) {
	std::lock_guard<std::mutex> lock(m_mtx);

	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&packet](User& user) {
		return ((user.getLogin() == packet.getLogin()) && (user.getPassword() == packet.getPassword()));
		});

	if (it != m_vec_users.end()) {
		snd::StatusPacket statusPacket;
		statusPacket.setStatus(Responce::AUTHORIZATION_SUCCESS);
		sendPacket(acceptSocket, statusPacket);
		it->setIsOnline(true); 
		it->setLastSeen("online");
		it->setSocketOnServer(acceptSocket);
		dispatchPreviousMessages(acceptSocket, packet);

		sendStatusToFriends(it, true); // true means online
	}

	else {
		snd::StatusPacket statusPacket;
		statusPacket.setStatus(Responce::AUTHORIZATION_FAIL);
		sendPacket(acceptSocket, statusPacket);
	}
}

void Server::sendStatusToFriends(std::vector<User>::iterator it_me, bool status) {
	std::vector<std::string> friendsLoginsVec = it_me->getUserFriendsVec();
	for (auto friendLogin : friendsLoginsVec) {
		auto itFriend = std::find_if(m_vec_users.begin(), m_vec_users.end(),
			[friendLogin](const User& user) {
				return user.getLogin() == friendLogin;
			});

		if (itFriend->getIsOnline()) {
			snd::FriendStatePacket packet;
			packet.setLogin(it_me->getLogin());
			packet.setIsOnline(status);
			if (status == true) {
				packet.setLastSeen("online");
			}
			else {
				packet.setLastSeen(it_me->getLastSeen());
			}
			sendPacket(itFriend->getSocketOnServer(), packet);
		}
	}
}

void Server::dispatchPreviousMessages(SOCKET acceptSocket, rcv::AuthorizationPacket& packet) {
	std::vector<rpl::Message> messages_vec = m_map_messages_to_send[packet.getLogin()];
	if (!messages_vec.empty()) {
		for (auto msg : messages_vec) {
			sendMessage(acceptSocket, msg);
		}
	}
	else {
		m_map_messages_to_send.erase(packet.getLogin());
	}
}

void Server::sendPacket(SOCKET acceptSocket, Packet& packet) {
	std::string serializedPacket =  packet.serialize();
	SizePacket sizePacket;
	sizePacket.setData(serializedPacket);
	std::string serializedSizePacket = sizePacket.serialize();
	send(acceptSocket, serializedSizePacket.c_str(), serializedSizePacket.size(), 0);
	send(acceptSocket, serializedPacket.c_str(), serializedPacket.size(), 0);
}

void Server::sendMessage(SOCKET acceptSocket, rpl::Message& message) {

	auto it = std::find_if(m_vec_users.begin(), m_vec_users.end(), [&message](User& user) {
		return user.getLogin() == message.getReceiverInfo().getLogin();
		});

	if (it != m_vec_users.end() && it->getIsOnline()) {
		
		std::string serializedMessage = message.serialize();
		SizePacket sizePacket;
		sizePacket.setData(serializedMessage);
		std::string serializedSizePacket = sizePacket.serialize();
		send(acceptSocket, serializedSizePacket.c_str(), strlen(serializedSizePacket.c_str()), 0);
		send(acceptSocket, serializedMessage.c_str(), strlen(serializedMessage.c_str()), 0);
	}

	else {
		auto itMessagesMap = m_map_messages_to_send.find(message.getReceiverInfo().getLogin());
		if (itMessagesMap == m_map_messages_to_send.end()) {
			std::vector<rpl::Message> messagesVec;
			messagesVec.push_back(message);
			m_map_messages_to_send[message.getReceiverInfo().getLogin()] = messagesVec;
		}
		else {
			std::vector<rpl::Message>& messagesVec = m_map_messages_to_send[message.getReceiverInfo().getLogin()];
			messagesVec.push_back(message);
		}
	}
}
