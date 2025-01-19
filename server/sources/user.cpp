#include"user.h"

std::string User::serialize() const {
    std::ostringstream oss;
    oss << m_login << '\n'
        << m_password << '\n'
        << m_name << '\n'
        << std::boolalpha << m_isOnline << '\n'
        << m_on_server_sock << '\n';
    return oss.str();
}

User User::deserialize(const std::string& str) {
    std::istringstream iss(str);
    std::string login;
    std::string password;
    std::string name;
    bool isOnline;
    int sock;

    std::getline(iss, login);          
    std::getline(iss, password);      
    std::getline(iss, name);           
    iss >> std::boolalpha >> isOnline; 
    iss >> sock;                      

    return User(login, password, name, isOnline, sock); 
}