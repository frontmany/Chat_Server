#pragma once
#include<string>
#include<sstream>

namespace req {

    struct SenderData {
        SenderData() {}
        SenderData(std::string Login) 
            : login(Login), name(""), password(""), needsRegistration(true) {}

        bool needsRegistration;
        std::string name;
        std::string login;
        std::string password;

        std::string serialize() const;
        static SenderData deserialize(const std::string& str);
    };

    struct ReceiverData {
        ReceiverData() 
            : login("") {};
        ReceiverData(std::string Login) 
            : login(Login) {};

        std::string login;

        std::string serialize() const;
        static ReceiverData deserialize(const std::string& str);
    };

    struct Packet {
        Packet()
            : isNewChat(true), msg(""), sender(""), receiver("") {};

        bool isNewChat;
        std::string msg;
        SenderData sender;
        ReceiverData receiver;

        std::string serialize() const;
        static Packet deserialize(const std::string& str);
    };


}