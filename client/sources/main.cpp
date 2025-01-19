#include<iostream>
#include<string>
#include<sstream>
#include<thread>
#include<winsock2.h>
#include<ws2tcpip.h>

#include"clientSide.h"


int main(int argc, char* argv[]) {
    ClientSide client;
    client.init();
    client.connectTo("192.168.1.49", 54000);
    

    if (client.registerClient("qwerty", "123", "joke")) {
        if (client.createChatWith("bob")) {
            req::Packet pack;
            pack.isNewChat = false;
            pack.msg = "hello there";
            req::SenderData sd("qwerty");
            sd.needsRegistration = false;
            sd.name = "joke";
            req::ReceiverData rd("bob");
            pack.sender = sd;
            pack.receiver = rd;

            client.sendMessage(pack);
        }
    }

    auto ch = client.getChat("bob");
    std::cout << ch->getLastIncomeMsg();

    return 0;
}
