#pragma once
#include<iostream>
#include<sstream>
#include<string>
#include<winsock2.h>
#include<ws2tcpip.h>

class User {
public:
	User(std::string login, std::string password, std::string name, bool isOnline, SOCKET socketOnServer)
		: m_login(login), m_password(password),
		m_name(name), m_isOnline(isOnline), m_on_server_sock(socketOnServer){};

	User(std::string login, std::string password, std::string name)
		: m_login(login), m_password(password),
		m_name(name), m_isOnline(false), m_on_server_sock(-1) {};

	User()
		: m_login(""), m_password(""),
		m_name(""), m_isOnline(false), m_on_server_sock(-1) {};
	
	std::string getLogin() const { return m_login; }
	std::string getPassword() const { return m_password; }
	std::string getName() const { return m_name; }
	SOCKET getSocketOnServer() const { return m_on_server_sock; }
	bool getStatus() const { return m_isOnline; }

	void setStatus(bool status) { m_isOnline = status; }
	void setSocketOnServer(int sock) { m_on_server_sock = sock; }

	std::string serialize() const;
	static User deserialize(const std::string& str);

private:
	bool				m_isOnline;
	SOCKET				m_on_server_sock;
	std::string			m_name;
	std::string			m_login;
	std::string			m_password;
};