#pragma once

#include<string>
#include<sstream>

#include "photo.h"

class Packet {
public:
    virtual ~Packet() = default;
    virtual std::string serialize() { return ""; };
};


class SizePacket : public Packet {
public:
    SizePacket() {}
    static size_t deserialize(const std::string& str);
    std::string serialize();
    void setData(std::string& data) { m_serialized_data_object = data; }

private:
    std::string m_serialized_data_object;
};

enum class Query {
    NEXT_QUERY_SIZE,
    AUTHORIZATION,
    REGISTRATION,
    CREATE_CHAT,
    GET_USER_INFO,
    UPDATE_USER_INFO,
    MESSAGE,
    GET_ALL_FRIENDS_STATES

};

namespace rpl {
    class UserInfoPacket : public Packet {
    public:
        UserInfoPacket() : m_isHasPhoto(false), m_isOnline(false) {}
        std::string serialize();
        static UserInfoPacket deserialize(const std::string& str);

        const std::string& getLogin() const { return m_user_login; }
        void setLogin(const std::string& login) { m_user_login = login; }

        const std::string& getName() const { return m_user_name; }
        void setName(const std::string& name) { m_user_name = name; }

        const std::string& getLastSeen() const { return m_last_seen; }
        void setLastSeen(const std::string& lastSeen) { m_last_seen = lastSeen; }

        const bool getIsOnline() const { return m_isOnline; }
        void setIsOnline(bool isOnline) { m_isOnline = isOnline; }

        const bool getIsHasPhoto() const { return m_isHasPhoto; }
        void setIsHasPhoto(bool isHasPhoto) { m_isHasPhoto = isHasPhoto; }

        const Photo& getPhoto() const { return m_user_photo; }
        void setPhoto(const Photo& photo) { m_user_photo = photo; }

    private:
        std::string m_user_login;
        std::string m_user_name;
        std::string	m_last_seen;
        Photo       m_user_photo;
        bool		m_isOnline;
        bool		m_isHasPhoto;

    };

    class Message {
    public:
        Message() {}
        std::string serialize();
        static Message deserialize(const std::string& str);

        const rpl::UserInfoPacket& getReceiverInfo() const { return m_receiver_info; }
        void setReceiverInfo(rpl::UserInfoPacket& info) { m_receiver_info = info; }

        const rpl::UserInfoPacket& getSenderInfo() const { return m_sender_info; }
        void setSenderInfo(rpl::UserInfoPacket& info) { m_sender_info = info; }

        const std::string& getMessage() const { return m_message; }
        void setMessage(std::string& message) { m_message = message; }

        const int getId() const { return m_id; }
        void setId(const int id) { m_id = id; }

    private:
        int m_id;
        rpl::UserInfoPacket m_receiver_info;
        rpl::UserInfoPacket m_sender_info;
        std::string m_message;
    };
}

namespace rcv {
    std::pair<Query, std::string> parseQuery(const std::string& query);

    class AuthorizationPacket {
    public:
        AuthorizationPacket(){}
        static AuthorizationPacket deserialize(const std::string& str);

        const std::string& getLogin() const { return m_login; }
        void setLogin(std::string& login) { m_login = login; }

        const std::string& getPassword() const { return m_password; }
        void setPassword(std::string& password) { m_password = password; }


    private:
        std::string m_login;
        std::string m_password; 
    };


    class RegistrationPacket {
    public:
        RegistrationPacket() {}
        static RegistrationPacket deserialize(const std::string& str);

        const std::string& getLogin() const { return m_login; }
        void setLogin(std::string& login) { m_login = login; }

        const std::string& getPassword() const { return m_password; }
        void setPassword(std::string& password) { m_password = password; }


        const std::string& getName() const { return m_name; }
        void setName(std::string& name) { m_name = name; }

    private:
        std::string m_login;
        std::string m_password;
        std::string m_name;
    };


    class CreateChatPacket {
    public:
        CreateChatPacket() {}
        static CreateChatPacket deserialize(const std::string& str);

        const std::string& getReceiverLogin() const { return m_receiver_login; }
        void setReceiverLogin(std::string& login) { m_receiver_login = login; }

        const std::string& getSenderLogin() const { return m_sender_login; }
        void setSenderLogin(std::string& login) { m_sender_login = login; }

    private:
        std::string m_receiver_login;
        std::string m_sender_login;
    };

    class GetUserInfoPacket {
    public:
        GetUserInfoPacket() {}
        static GetUserInfoPacket deserialize(const std::string& str);

        std::string& getLogin() { return m_user_login; }
        void setLogin(std::string& login) { m_user_login = login; }

    private:
        std::string m_user_login;
    };


    class UpdateUserInfoPacket {
    public:
        UpdateUserInfoPacket() {}
        static UpdateUserInfoPacket deserialize(const std::string& str);

        const std::string& getLogin() const { return m_user_login; }
        void setLogin(const std::string& login) { m_user_login = login; }

        const std::string& getName() const { return m_user_name; }
        void setName(const std::string& name) { m_user_name = name; }

        const std::string& getPassword() const { return m_user_password; }
        void setPassword(const std::string& password) { m_user_password = password; }

        const Photo& getPhoto() const { return m_user_photo; }
        void setPhoto(const Photo& photo) { m_user_photo = photo; }

    private:
        std::string m_user_login;
        std::string m_user_name;
        std::string m_user_password;
        Photo       m_user_photo;
    };

    class GetFriendsStatusesPacket {
    public:
        GetFriendsStatusesPacket() {}
        static GetFriendsStatusesPacket deserialize(const std::string& str);

        std::vector<std::string>& getFriendsLoginsVec() { return m_vec_friends_logins; }
        void setFriendsLoginsVec(const std::vector<std::string>& friendsLoginsVec) { m_vec_friends_logins = friendsLoginsVec; }

    private:
        std::vector<std::string> m_vec_friends_logins;
    };

}



enum class Responce {
    EMPTY_RESPONSE,
    AUTHORIZATION_SUCCESS,
    REGISTRATION_SUCCESS,
    AUTHORIZATION_FAIL,
    REGISTRATION_FAIL,
    CHAT_CREATE_SUCCESS,
    CHAT_CREATE_FAIL,
    USER_INFO_FOUND,
    USER_INFO_NOT_FOUND,
    USER_INFO_UPDATED,
    USER_INFO_NOT_UPDATED,
    FRIEND_STATE_CHANGED,
    ALL_FRIENDS_STATES,
};

namespace snd {
    class StatusPacket : public Packet {
    public:
        StatusPacket() : m_status(Responce::EMPTY_RESPONSE) {}
        std::string serialize();

        Responce getStatus() { return m_status; }
        void setStatus(Responce status) { m_status = status; }

    private:
        Responce m_status;
    };

    class FriendStatePacket : public Packet {
    public:
        FriendStatePacket() : m_isOnline(false) {}
        std::string serialize();

        const std::string& getLogin() const { return m_friend_login; }
        void setLogin(const std::string& login) { m_friend_login = login; }

        const std::string& getLastSeen() const { return m_last_seen; }
        void setLastSeen(const std::string& lastSeen) { m_last_seen = lastSeen; }

        const bool getIsOnline() const { return m_isOnline; }
        void setIsOnline(const bool isOnline) { m_isOnline = isOnline; }

    private:
        std::string  m_friend_login;
        std::string  m_last_seen;
        bool         m_isOnline;
    };

    class ChatSuccessPacket : public Packet {
    public:
        ChatSuccessPacket() {}
        std::string serialize();

        const rpl::UserInfoPacket& getUserInfoPacket() const { return m_packet; }
        void setUserInfoPacket(const rpl::UserInfoPacket& info) { m_packet = info; }

    private:
        rpl::UserInfoPacket m_packet;
    };

    class FriendsStatusesPacket : public Packet {
    public:
        FriendsStatusesPacket() {}
        std::string serialize();

        std::vector<std::pair<std::string, std::string>>& getVecStatuses() { return m_vec_statuses; }

    private:
        std::vector<std::pair<std::string, std::string>> m_vec_statuses;
    };
}