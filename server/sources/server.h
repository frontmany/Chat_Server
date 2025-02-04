#pragma once

#include<iostream>
#include<sstream>
#include<thread>
#include<fstream>
#include<unordered_map>
#include<string>
#include<vector>
#include <mutex>
#include <queue>
#include<winsock2.h>
#include<ws2tcpip.h>

#include"user.h" 
#include"request.h" 

std::string getCurrentTime();

class Server {
public:
    Server();
    void init(const std::string& ipAddress, int port);
    void run();


private:
    void sendPacket(SOCKET acceptSocket, Packet& packet);
    void redirectMessage(rpl::Message* message);
    void sendMessagesReadIds(SOCKET acceptSocket, rpl::MessagesReadPacket& messagesReadPacket);
    void sendStatusToFriends(SOCKET acceptSocket, bool status);

    std::vector<rpl::Message*> dispatchPreviousMessages(SOCKET acceptSocket, const std::string& myLogin);
    snd::MessagesIdsPacket* dispatchPreviousMessagesReadIds(SOCKET acceptSocket, const std::string& myLogin);
    snd::FriendsStatusesPacket* dispatchAllFriendsStates(SOCKET acceptSocket, const std::string& myLogin);

    void onReceiving(SOCKET acceptSocket);
    snd::StatusPacket* authorizeUser(SOCKET acceptSocket, rcv::AuthorizationPacket& packet);
    void registerUser(SOCKET acceptSocket, rcv::RegistrationPacket& packet);
    void createChat(SOCKET acceptSocket, rcv::CreateChatPacket& packet);
    void findUserInfo(SOCKET acceptSocket, rcv::GetUserInfoPacket& packet);
    void updateUserInfo(SOCKET acceptSocket, rcv::UpdateUserInfoPacket& packet);
    void onDisconnect(SOCKET acceptSocket);


private:
    std::vector<std::thread>                  m_vec_threads;
    std::vector<SOCKET>		                  m_vec_sockets;
    std::vector<User>		                  m_vec_users;
    std::mutex                                m_mtx;
    SOCKET					                  m_listener_socket;
    std::string				                  m_ipAddress;
    int						                  m_port;

    std::unordered_map<std::string, std::vector<rpl::Message*>> m_map_messages_to_send;

                    //receiver login       //sender login       //message id
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::vector<double>>>> m_map_read_messages_ids_to_send;
};