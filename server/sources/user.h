#pragma once

#include<iostream>
#include<sstream>
#include<string>
#include<winsock2.h>
#include<ws2tcpip.h>
#include <fstream>
#include <cstring>
#include <chrono>
#include <ctime>

#include "photo.h"


class User {
public:
    User(std::string login, std::string password, std::string name, SOCKET socketOnServer)
        : m_login(login), m_password(password),
        m_name(name), m_isOnline(true), m_on_server_sock(socketOnServer) {}

    const SOCKET getSocketOnServer() const { return m_on_server_sock; }
    void setSocketOnServer(const int sock) { m_on_server_sock = sock; }

    const std::string& getLogin() const { return m_login; }
    void setLogin(const std::string& login) { m_login = login; }

    const std::string& getPassword() const { return m_password; }
    void setPassword(const std::string& password) { m_password = password; }

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    const bool getIsOnline() const { return m_isOnline; }
    void setIsOnline(const bool status) { m_isOnline = status; }

    const Photo& getPhoto() const { return m_photo; }
    void setPhoto(const Photo& photo) { m_photo = photo; m_isHasPhoto = true; }

    const bool getIsHasPhoto() const { return m_isHasPhoto; }

    const std::string& getLastSeen() const { return m_lastSeen; }
    void setLastSeen(const std::string& lastSeen) { m_lastSeen = lastSeen; }

    void setLastSeenToNow();

private:
    bool				m_isOnline;
    bool				m_isHasPhoto = false;

    std::string			m_lastSeen;
    std::string			m_name;
    std::string			m_login;
    std::string			m_password;
    Photo			    m_photo;
    std::vector<User*>  m_vec_user_friends;
    SOCKET				m_on_server_sock;
};

