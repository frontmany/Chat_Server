#pragma once
#include<iostream>
#include<vector>
#include<algorithm>
#include<thread>
#include<mutex>
#include<winsock2.h>
#include<ws2tcpip.h>

#include"request.h"
#include"chat.h"

enum class ServerResponse {
	AUTHORIZATION_SUCCESS,
	REGISTRATION_SUCCESS,
	AUTHORIZATION_FAIL,
	REGISTRATION_FAIL,
	CHAT_CREATE_SUCCESS,
	CHAT_CREATE_FAIL,
	USER_INFO_FOUND,
	USER_INFO_NOT_FOUND
};

enum class UserRequest {
	GET_USER_INFO
};


class ClientSide {
public:
	ClientSide() : m_socket(-1), m_server_IpAddress(""),
		m_server_port(-1), m_isAuthorized(NOT_STATED) {};

	void init();
	void connectTo(std::string ipAddress, int port);
	bool authorizeClient(std::string login, std::string password);
	bool registerClient(std::string login, std::string password, std::string name);
	void sendMessage(req::Packet pack);
	bool createChatWith(std::string receiverLogin);

	req::SenderData& getMyInfo() { return m_my_data; }
	std::vector<Chat>::iterator getChat(std::string receiverLogin);

private:
	SOCKET				m_socket;
	std::string			m_server_IpAddress;
	int					m_server_port;
	State				m_isAuthorized;
	req::SenderData		m_my_data;

	std::thread			m_receiverThread;
	std::vector<Chat>	m_vec_chats;

private:
	void receive();
	void sendToServer(std::string msg);
};