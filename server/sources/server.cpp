#include "Server.h"
#include "hasher.h"
#include <iostream>
#include <algorithm>


Server::Server()
    : m_acceptor(m_io_context),
    m_thread_pool(std::thread::hardware_concurrency()), m_port(-1), m_db(Database()) {}

void Server::init(const std::string& ipAddress, int port) {
    m_db.init();
    m_port = port;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ipAddress), m_port);
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(asio::socket_base::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
}

void Server::run() {
    acceptConnections();
    for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
        m_vec_threads.emplace_back([this]() { m_io_context.run(); });
    }
    for (auto& thread : m_vec_threads) {
        thread.join();
    }
}

void Server::acceptConnections() {
    auto socket = std::make_shared<asio::ip::tcp::socket>(m_io_context);
    m_acceptor.async_accept(*socket, [this, socket](std::error_code ec) {
        if (!ec) {
            onAccept(socket);
        }
        acceptConnections(); // Continue accepting new connections
        });
}

void Server::startAsyncRead(std::shared_ptr<asio::ip::tcp::socket> socket) {
    auto buffer = std::make_shared<asio::streambuf>();
    asio::async_read_until(*socket, *buffer, "_+14?bb5HmR;%@`7[S^?!#sL8",
        [this, socket, buffer](const asio::error_code& ec, std::size_t bytes_transferred) {
            handleRead(ec, bytes_transferred, socket, buffer);
        });
}

void Server::onAccept(std::shared_ptr<asio::ip::tcp::socket> socket) {
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        std::cout << "client connected on socket:" <<  socket << '\n';
    }
    startAsyncRead(socket);
}

void Server::handleRead(const asio::error_code& ec, std::size_t bytes_transferred,
    std::shared_ptr<asio::ip::tcp::socket> socket, std::shared_ptr<asio::streambuf> buffer) {
    if (ec) {
        if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            std::lock_guard<std::mutex> lock(m_mtx);
            std::cout << "Client disconnected: " << socket->remote_endpoint() << std::endl;
            
            auto it = std::find_if(m_map_online_users.begin(), m_map_online_users.end(), [socket](const std::pair<std::string, User*> pair) {
                return pair.second->getSocketOnServer() == socket;
                });
            if (it != m_map_online_users.end()) {
                User* user = it->second;
                m_map_online_users.erase(user->getLogin());
            }
            return; 
        }
        else {
            std::cerr << "Read error: " << ec.message() << std::endl;
            return; 
        }
    }

    std::istream is(buffer.get());
    std::string packet((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    std::istringstream iss(packet);
    std::string classificationStr;
    std::getline(iss, classificationStr);

    std::string remainingStr = rebuildRemainingStringFromIss(iss);
    if (classificationStr == "GET") {
        handleGet(socket, remainingStr);
    }
    else if (classificationStr == "RPL") {
        handleRpl(socket, remainingStr);
    }
    else if (classificationStr == "BROADCAST") {
        handleBroadcast(socket, remainingStr);
    }

    startAsyncRead(socket);
}

void Server::handleBroadcast(std::shared_ptr<asio::ip::tcp::socket> socket, std::string packet) {
    std::istringstream iss(packet);

    std::string typeStr;
    std::getline(iss, typeStr);

    std::string remainingStr = rebuildRemainingStringFromIss(iss);
    if (typeStr == "STATUS") {
        broadcastUserInfo(socket, remainingStr);
    }
}

void Server::broadcastUserInfo(std::shared_ptr<asio::ip::tcp::socket> acceptSocket, std::string packet) {
    std::istringstream iss(packet);

    std::string status;
    std::getline(iss, status);

    std::string login;
    std::getline(iss, login);

    std::string line;
    while (std::getline(iss, line)) {
        if (line == "VEC_BEGIN") {
            continue;
        }
        if (line == "VEC_END") {
            return;
        }
        else {
            auto it = m_map_online_users.find(line);
            if (it == m_map_online_users.end()) {
                continue; 
            }
            else {
                sendResponse(it->second->getSocketOnServer(), m_sender.get_statusStr(login, status));
            }
        }
    }
}

void Server::handleGet(std::shared_ptr<asio::ip::tcp::socket> socket, std::string packet) {
    std::istringstream iss(packet);

    std::string typeStr;
    std::getline(iss, typeStr);


    std::string remainingStr = rebuildRemainingStringFromIss(iss);
    if (typeStr == "AUTHORIZATION") {
        authorizeUser(socket, remainingStr);
    }
    else if (typeStr == "REGISTRATION") {
        registerUser(socket, remainingStr);
    }
    else if (typeStr == "CREATE_CHAT") {
        createChat(socket, remainingStr);
    }
    else if (typeStr == "UPDATE_MY_INFO") {
        updateUserInfo(socket, remainingStr);
    }
    else if (typeStr == "LOAD_FRIEND_INFO") {
        returnUserInfo(socket, remainingStr);
    }
}

void Server::handleRpl(std::shared_ptr<asio::ip::tcp::socket> socket, std::string packet) {
    std::istringstream iss(packet);

    std::string friendLogin;
    std::getline(iss, friendLogin);

    std::string type;
    std::getline(iss, type);

    auto it = m_map_online_users.find(friendLogin);

    if (it == m_map_online_users.end()) {
        if (type == "MESSAGE") {
            m_db.collect(friendLogin, "MESSAGE\n" + iss.str());
            sendResponse(socket, m_sender.get_messageSuccessStr());
        }
        else if (type == "MESSAGE_READ_CONFIRMATION") {
            m_db.collect(friendLogin, "MESSAGE_READ_CONFIRMATION\n" + iss.str());
            sendResponse(socket, m_sender.get_messageReadConfirmationSuccessStr());
        }
    }
    else {
        User* user = it->second;

        if (type == "MESSAGE") {
            sendResponse(user->getSocketOnServer(), "MESSAGE\n" + iss.str());
            sendResponse(socket, m_sender.get_messageSuccessStr());
        }
        else if (type == "FIRST_MESSAGE") {
            sendResponse(user->getSocketOnServer(), "FIRST_MESSAGE\n" + iss.str());
            sendResponse(socket, m_sender.get_messageSuccessStr());
        }
        else if (type == "MESSAGES_READ_CONFIRMATION") {
            sendResponse(user->getSocketOnServer(), "MESSAGES_READ_CONFIRMATION\n" + iss.str());
            sendResponse(socket, m_sender.get_messageReadConfirmationSuccessStr());
        }
    }
}

std::string Server::rebuildRemainingStringFromIss(std::istringstream& iss) {
    std::string remainingStr;
    std::string line;
    while (std::getline(iss, line)) {
        remainingStr += line + '\n';
    }
    remainingStr.pop_back();
    return remainingStr;
}

void Server::returnUserInfo(std::shared_ptr<asio::ip::tcp::socket> socket, std::string packet) {
    std::istringstream iss(packet);
    std::string login;
    std::getline(iss, login);

    User* user = m_db.getUser(login, socket);
    if (user == nullptr) {
        std::string response = m_sender.get_userInfoFailStr();
        sendResponse(socket, response);
    }
    else {
        std::string response;
        auto it = m_map_online_users.find(user->getLogin());
        if (it == m_map_online_users.end()) {
            response = m_sender.get_userInfoSuccessStr(user);
        }
        else {
            response = m_sender.get_userInfoSuccessStr(it->second);
        }
        sendResponse(socket, response);
    }
}

void Server::authorizeUser(std::shared_ptr<asio::ip::tcp::socket> acceptSocket, std::string packet) {
    std::istringstream iss(packet);

    std::string login;
    std::getline(iss, login);

    std::string password;
    std::getline(iss, password);

    bool isAuthorized = m_db.checkPassword(login, password);
    if (isAuthorized) {
        User* user = m_db.getUser(login, acceptSocket);
        if (user == nullptr) {
            std::cout << "can't get user info";
        }
        else {
            user->setLastSeen("online");
            m_map_online_users[login] = user;

            std::vector<std::string> statusesVec = m_db.getUsersStatusesVec(user->getUserFriendsVec(), m_map_online_users);
            std::string response = m_sender.get_authorizationSuccessStr(user->getUserFriendsVec(), statusesVec);
            sendResponse(user->getSocketOnServer(), response);

            for (auto pack : m_db.getCollected(login)) {
                sendResponse(user->getSocketOnServer(), pack);
            }
        }
    }
    else {
        std::string response = m_sender.get_authorizationFailStr();
        sendResponse(acceptSocket, response);
    }
}

void Server::registerUser(std::shared_ptr<asio::ip::tcp::socket> acceptSocket, std::string packet) {
    std::istringstream iss(packet);

    std::string login;
    std::getline(iss, login);

    std::string name;
    std::getline(iss, name);

    std::string password;
    std::getline(iss, password);

    if (m_db.getUser(login) == nullptr) {
        std::string passwordHash = hash::hashPassword(password);
        User* user = new User(login, passwordHash, name, false, Photo(), acceptSocket);
        user->setLastSeenToOnline();
        m_map_online_users[login] = user;

        std::string response = m_sender.get_registrationSuccessStr();
        sendResponse(acceptSocket, response);
        m_db.addUser(login, name, user->getLastSeen(), passwordHash);
    }
    else {
        std::string response = m_sender.get_registrationFailStr();
        sendResponse(acceptSocket, response);
    }
}

void Server::createChat(std::shared_ptr<asio::ip::tcp::socket> acceptSocket, std::string packet) {
    std::istringstream iss(packet);

    std::string myLogin;
    std::getline(iss, myLogin);

    std::string friendLogin;
    std::getline(iss, friendLogin);

    std::string response;

    if (myLogin == friendLogin) {
        response = m_sender.get_chatCreateFailStr();
    }
    else {
        User* user = m_db.getUser(friendLogin);
        if (user == nullptr) {
            response = m_sender.get_chatCreateFailStr();
        }
        else {
            response = m_sender.get_chatCreateSuccessStr(user);

            auto it = m_map_online_users.find(friendLogin);
            if (it == m_map_online_users.end()) {
                std::cerr << "User " << friendLogin << " not found in online users." << std::endl;
            }
            else
            {
                sendResponse(it->second->getSocketOnServer(), m_sender.get_loginToSendStatusStr(myLogin));
                
            }
        }
    }

    sendResponse(acceptSocket, response);
}

void Server::updateUserInfo(std::shared_ptr<asio::ip::tcp::socket> acceptSocket, std::string packet) {
    std::istringstream iss(packet);

    std::string myLogin;
    std::getline(iss, myLogin);

    std::string name;
    std::getline(iss, name);

    std::string password;
    std::getline(iss, password);

    std::string isHasPhotoStr;
    std::getline(iss, isHasPhotoStr);
    bool isHasPhoto = isHasPhotoStr == "true";

    Photo photo;
    if (isHasPhoto) {
        std::string photoStr;
        std::getline(iss, photoStr);
        photo = Photo::deserialize(photoStr);
    }

    if (isHasPhoto) {
        m_db.updateUser(myLogin, name, password, isHasPhoto, photo);
    }
    else {
        m_db.updateUser(myLogin, name, password, isHasPhoto);
    }

    sendResponse(acceptSocket, m_sender.get_userInfoUpdatedSuccessStr());
}

void Server::sendResponse(std::shared_ptr<asio::ip::tcp::socket> socket, std::string response) {
    asio::async_write(*socket, asio::buffer(response.data(), response.size()),
        [socket](const asio::error_code& error, std::size_t bytes_transferred) {
            if (error) {
                std::cerr << "Send error: " << error.message() << std::endl;
                // Handle the error (e.g., close the socket)
            }
            else {
                std::cout << "Sent " << bytes_transferred << " bytes to " << socket->remote_endpoint() << std::endl;
            }
        });
}
