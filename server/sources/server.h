#pragma once

#include<iostream>
#include<sstream>
#include<thread>
#include<fstream>
#include<unordered_map>
#include<string>
#include<vector>
#include <mutex>
#include<winsock2.h>
#include<ws2tcpip.h>

#include"user.h" 
#include"request.h" 

class Server {
public:
    Server();
    void init(const std::string& ipAddress, int port);
    void run();


private:
    void sendPacket(SOCKET acceptSocket, Packet& packet);
    void sendMessage(SOCKET acceptSocket, rpl::Message& message);

    void onReceiving(SOCKET acceptSocket);
    void authorizeUser(SOCKET acceptSocket, rcv::AuthorizationPacket& packet);
    void registerUser(SOCKET acceptSocket, rcv::RegistrationPacket& packet);
    void createChat(SOCKET acceptSocket, rcv::CreateChatPacket& packet);
    void findUserInfo(SOCKET acceptSocket, rcv::GetUserInfoPacket& packet);
    void updateUserInfo(SOCKET acceptSocket, rcv::UpdateUserInfoPacket& packet);


private:
    std::vector<std::thread>    m_vec_threads;
    std::vector<SOCKET>		    m_vec_sockets;

    std::vector<User>		    m_vec_users;
    std::vector<User>		    m_vec_online_users;

    std::mutex                  m_mtx;
    SOCKET					    m_listener_socket;
    std::string				    m_ipAddress;
    int						    m_port;

    std::unordered_map<std::string, std::vector<rpl::Message>> m_map_messages_to_send;
};