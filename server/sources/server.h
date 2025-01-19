#pragma once
#include<iostream>
#include<sstream>
#include<thread>
#include<unordered_map>
#include<string>
#include<vector>
#include <mutex>
#include<winsock2.h>
#include<ws2tcpip.h>

#include"user.h" 
#include"request.h" 


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

class Server {
public:
    Server();
    void init(std::string ipAddress, int port);
    void run();


private:
    std::vector<std::thread>    m_vec_threads;
    std::vector<SOCKET>		    m_vec_sockets;

    std::vector<User>		    m_vec_users;
    std::vector<User>		    m_vec_online_users;

    std::mutex                  m_mtx;
    SOCKET					    m_listener_socket;
    std::string				    m_ipAddress;
    int						    m_port;

    std::unordered_map<std::string, std::vector<std::string>> m_map_messages_to_send;

private:
    void sendBool(SOCKET acceptSocket, bool value);
    void sendStatus(SOCKET acceptSocket, ServerResponse response);
    void sendPacket(SOCKET acceptSocket, req::Packet packet);


    void authorizeClient(int acceptSocket);
    void onReceiving(int acceptSocket);
};