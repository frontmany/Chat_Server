#include"request.h"

using namespace req;

std::string SenderData::serialize() const {
    std::ostringstream oss;
    oss << std::boolalpha << needsRegistration << '\n'
        << name << '\n'
        << login << '\n'
        << password;
    return oss.str();
}

SenderData SenderData::deserialize(const std::string& str) {
    std::istringstream iss(str);
    SenderData data("");
    iss >> std::boolalpha >> data.needsRegistration;
    iss.ignore(); 
    std::getline(iss, data.name);
    std::getline(iss, data.login);
    std::getline(iss, data.password);
    return data;
}

std::string ReceiverData::serialize() const {
    std::ostringstream oss;
    oss << login << '\n';
    return oss.str();
}

ReceiverData ReceiverData::deserialize(const std::string& str) {
    std::istringstream iss(str);
    ReceiverData data;
    std::getline(iss, data.login);
    return data;
}

std::string Packet::serialize() const {
    std::ostringstream oss;
    oss << std::boolalpha << isNewChat << '\n'
        << msg << '\n'
        << sender.serialize() << '\n'
        << receiver.serialize();
    return oss.str();
}

Packet Packet::deserialize(const std::string& str) {
    std::istringstream iss(str);
    Packet packet;

    std::string s1;
    std::string s2;
    std::string s3;
    std::string s4;


    std::string receiverData;

    iss >> std::boolalpha >> packet.isNewChat;
    iss.ignore(); 
    std::getline(iss, packet.msg);

    std::getline(iss, s1);
    std::getline(iss, s2);
    std::getline(iss, s3);
    std::getline(iss, s4);
    std::string finStr = s1 + "\n" + s2 + "\n" + s3 + "\n" + s4;

    std::getline(iss, receiverData);

    packet.sender = SenderData::deserialize(finStr);
    packet.receiver = ReceiverData::deserialize(receiverData);

    return packet;
}