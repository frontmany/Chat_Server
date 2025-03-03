#include"request.h"

std::pair<Query, std::string> rcv::parseQuery(const std::string& query) {
    std::istringstream stream(query);
    std::string firstLine;

    if (std::getline(stream, firstLine)) {
        Query queryType;

        if (firstLine == "NEXT_QUERY_SIZE") {
            queryType = Query::NEXT_QUERY_SIZE;
        }
        else if (firstLine == "AUTHORIZATION") {
            queryType = Query::AUTHORIZATION;
        }
        else if (firstLine == "REGISTRATION") {
            queryType = Query::REGISTRATION;
        }
        else if (firstLine == "CREATE_CHAT") {
            queryType = Query::CREATE_CHAT;
        }
        else if (firstLine == "GET_USER_INFO") {
            queryType = Query::GET_USER_INFO;
        }
        else if (firstLine == "UPDATE_USER_INFO") {
            queryType = Query::UPDATE_USER_INFO;
        }
        else if (firstLine == "MESSAGE") {
            queryType = Query::MESSAGE;
        }
        else if (firstLine == "MESSAGES_READ_PACKET") {
            queryType = Query::MESSAGES_READ_PACKET;
        }
        else {
            throw std::runtime_error("Unknown query type!");
        }

        std::ostringstream remainingStream;
        std::string line;
        while (std::getline(stream, line)) {
            remainingStream << line;
            remainingStream << "\n";
        }

        std::string remainingPart = remainingStream.str();
        if (!remainingPart.empty() && remainingPart.back() == '\n') {
            remainingPart.pop_back();
        }

        return std::make_pair(queryType, remainingPart);
    }
    else {
        throw std::runtime_error("Failed to parse query!");
    }
}

std::string SizePacket::serialize() {
    std::size_t length = m_serialized_data_object.size();
    std::string lengthStr = std::to_string(length);
    std::ostringstream oss;
    oss << "NEXT_QUERY_SIZE" << "\n"
        << lengthStr;
    return oss.str();
}

size_t SizePacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    std::string sizeStr;
    std::getline(iss, sizeStr);
    std::size_t value = std::stoul(sizeStr);
    return value;
}



using namespace rcv;
AuthorizationPacket AuthorizationPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    AuthorizationPacket  pack;
    std::getline(iss, pack.m_login);
    std::getline(iss, pack.m_password);
    return pack;
}

RegistrationPacket RegistrationPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    RegistrationPacket  pack;
    std::getline(iss, pack.m_login);
    std::getline(iss, pack.m_password);
    std::getline(iss, pack.m_name);
    return pack;
}

CreateChatPacket CreateChatPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    CreateChatPacket  pack;
    std::getline(iss, pack.m_sender_login);
    std::getline(iss, pack.m_receiver_login);
    return pack;
}

GetUserInfoPacket GetUserInfoPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    GetUserInfoPacket  pack;
    std::getline(iss, pack.m_user_login);
    return pack;
}

UpdateUserInfoPacket UpdateUserInfoPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    UpdateUserInfoPacket  pack;
    std::getline(iss, pack.m_user_login);
    std::getline(iss, pack.m_user_name);
    std::getline(iss, pack.m_user_password);

    std::string str_to_deSerializePhoto;
    std::getline(iss, str_to_deSerializePhoto);
    Photo photo = Photo::deserialize(str_to_deSerializePhoto);
    pack.setPhoto(photo);

    return pack;
}

using namespace rpl;
std::string MessagesReadPacket::serialize() const {
    std::ostringstream oss;
    oss << "MESSAGES_READ_PACKET" << "\n"
        << m_sender_login << '\n'
        << m_receiver_login << '\n';

    for (const auto& id : m_read_messages_id_vec) {
        oss << id << ' ';
    }
    oss << '\n';

    return oss.str();
}

MessagesReadPacket MessagesReadPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    std::string line;
    MessagesReadPacket packet;

    std::getline(iss, packet.m_sender_login);
    std::getline(iss, packet.m_receiver_login);

    std::getline(iss, line);
    std::istringstream lineStream(line);
    double messageId;
    while (lineStream >> messageId) {
        packet.m_read_messages_id_vec.push_back(messageId);
    }

    return packet;
}

Message* Message::deserialize(const std::string& str) {
    std::istringstream iss(str);
    Message* message = new Message;
    std::string line;

    std::getline(iss, line);
    double id = 0;
    std::from_chars(line.data(), line.data() + line.size(), id);
    message->m_id = id;

    std::getline(iss, message->m_timeStamp);

    std::string msg;
    while (std::getline(iss, line)) {
        if (line == "|MSG_END|") {
            break;
        }
        msg += line + "\n";

    }
    message->m_message = msg;

    std::ostringstream remainingStream1;
    std::ostringstream remainingStream2;

    bool fl = true;
    while (std::getline(iss, line)) {
        if (line != ":") {
            if (fl) {
                remainingStream1 << line << "\n";
            }
            else {
                remainingStream2 << line << "\n";
            }
        }
        else {
            fl = false;
        }
    }

    std::string remainingPart1 = remainingStream1.str();
    UserInfoPacket packSender = UserInfoPacket::deserialize(remainingPart1);

    std::string remainingPart2 = remainingStream2.str();
    UserInfoPacket packReceiver = UserInfoPacket::deserialize(remainingPart2);


    //swap buffers
    message->setReceiverInfo(packSender);
    message->setSenderInfo(packReceiver);

    return message;
}

std::string Message::serialize() {

    std::ostringstream oss;
    oss << "MESSAGE" << "\n";
    oss << m_id << '\n'
        << m_timeStamp << '\n'
        << m_message << '\n'
        << "|MSG_END|" << '\n'
        << m_sender_info.serialize() << '\n'
        << ":" << '\n'
        << m_receiver_info.serialize();
    return oss.str();
}

std::string UserInfoPacket::serialize() {
    std::ostringstream oss;
    oss << "USER_INFO_FOUND" << "\n"
        << m_user_login << '\n'
        << m_user_name << '\n'
        << m_last_seen << '\n';

    std::string photo_serialized_str = m_user_photo.serialize();
    oss << photo_serialized_str << '\n';

    oss << (m_isOnline ? "true" : "false") << '\n'
        << (m_isHasPhoto ? "true" : "false");

    return oss.str();
}

UserInfoPacket UserInfoPacket::deserialize(const std::string& str) {
    std::istringstream iss(str);
    std::string line;
    UserInfoPacket packet;

    std::string lineType; // reads USER_INFO_FOUND
    std::getline(iss, lineType);
    std::getline(iss, packet.m_user_login);
    std::getline(iss, packet.m_user_name);
    std::getline(iss, packet.m_last_seen);

    std::string photo_serialized_str;
    std::getline(iss, photo_serialized_str);
    packet.m_user_photo = Photo::deserialize(photo_serialized_str);

    std::getline(iss, line);
    packet.m_isOnline = (line == "true");

    std::getline(iss, line);
    packet.m_isHasPhoto = (line == "true");

    return packet;
}


using namespace snd;
std::string StatusPacket::serialize() {
    std::string response;
    switch (m_status) {
    case Responce::EMPTY_RESPONSE:
        response = "EMPTY_RESPONSE";
        break;
    case Responce::AUTHORIZATION_SUCCESS:
        response = "AUTHORIZATION_SUCCESS";
        break;
    case Responce::REGISTRATION_SUCCESS:
        response = "REGISTRATION_SUCCESS";
        break;
    case Responce::AUTHORIZATION_FAIL:
        response = "AUTHORIZATION_FAIL";
        break;
    case Responce::REGISTRATION_FAIL:
        response = "REGISTRATION_FAIL";
        break;
    case Responce::CHAT_CREATE_FAIL:
        response = "CHAT_CREATE_FAIL";
        break;
    case Responce::USER_INFO_FOUND:
        response = "USER_INFO_FOUND";
        break;
    case Responce::USER_INFO_NOT_FOUND:
        response = "USER_INFO_NOT_FOUND";
        break;
    case Responce::USER_INFO_UPDATED:
        response = "USER_INFO_UPDATED";
        break;
    case Responce::USER_INFO_NOT_UPDATED:
        response = "USER_INFO_NOT_UPDATED";
        break;
    case Responce::ALL_FRIENDS_STATES:
        response = "ALL_FRIENDS_STATES";
        break;
    }
    return response;
}

std::string MessagesIdsPacket::serialize() {
    std::ostringstream oss;
    oss << "MESSAGES_IDS_PACKET" << "\n";
    std::size_t count = m_messagesIds.size();
    oss << count << "\n";
    for (auto p : m_messagesIds) {
        oss << p.first << "\n";
        for (auto mId : p.second) {
            oss << mId << "\n";
        }
        oss << "|" << "\n";
    }

    return oss.str();
}

std::string FriendStatePacket::serialize() {
    std::ostringstream oss;
    oss << "FRIEND_STATE_CHANGED" << "\n"
        << m_friend_login << '\n'
        << m_last_seen << '\n';
    oss << (m_isOnline ? "true" : "false");
    return oss.str();
}

std::string FriendsStatusesPacket::serialize() {
    std::ostringstream oss;
    oss << "ALL_FRIENDS_STATES" << "\n";
    for (const auto& p :m_vec_statuses) {
        oss << p.first << ":" << p.second << ","; 
    }

    std::string result = oss.str();
    if (!result.empty()) {
        result.pop_back(); 
    }

    return result;
}

std::string ChatSuccessPacket::serialize() {
    std::ostringstream oss;
    oss << "CHAT_CREATE_SUCCESS" << "\n";
    oss << m_packet.serialize();
    return oss.str();
}