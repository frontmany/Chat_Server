#include"database.h" 
#include"user.h"  


void Database::init() {
    m_sqlite3_dll = LoadLibrary("sqlite3.dll");
    if (!m_sqlite3_dll) {
        std::cerr << "loading error sqlite3.dll" << std::endl;
        return;
    }

    sqlite3_open = (sqlite3_open_t)GetProcAddress(m_sqlite3_dll, "sqlite3_open");
    sqlite3_exec = (sqlite3_exec_t)GetProcAddress(m_sqlite3_dll, "sqlite3_exec");
    sqlite3_close = (sqlite3_close_t)GetProcAddress(m_sqlite3_dll, "sqlite3_close");
    sqlite3_errmsg = (sqlite3_errmsg_t)GetProcAddress(m_sqlite3_dll, "sqlite3_errmsg");
    sqlite3_changes = (sqlite3_changes_t)GetProcAddress(m_sqlite3_dll, "sqlite3_changes");
    sqlite3_free = (sqlite3_free_t)GetProcAddress(m_sqlite3_dll, "sqlite3_free");
    sqlite3_prepare_v2 = (sqlite3_prepare_v2_t)GetProcAddress(m_sqlite3_dll, "sqlite3_prepare_v2");
    sqlite3_bind_text = (sqlite3_bind_text_t)GetProcAddress(m_sqlite3_dll, "sqlite3_bind_text");
    sqlite3_step = (sqlite3_step_t)GetProcAddress(m_sqlite3_dll, "sqlite3_step");
    sqlite3_column_text = (sqlite3_column_text_t)GetProcAddress(m_sqlite3_dll, "sqlite3_column_text");
    sqlite3_column_int = (sqlite3_column_int_t)GetProcAddress(m_sqlite3_dll, "sqlite3_column_int");
    sqlite3_finalize = (sqlite3_finalize_t)GetProcAddress(m_sqlite3_dll, "sqlite3_finalize");
   

    if (!sqlite3_open || !sqlite3_exec || !sqlite3_close || !sqlite3_errmsg || !sqlite3_changes) {
        std::cerr << "error obtaining function addresses SQLite" << std::endl;
        FreeLibrary(m_sqlite3_dll);
        return;
    }

    char* zErrMsg = 0;
    int rc;
    const char* sql1;

    rc = sqlite3_open("Database.db", &m_db);
    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(m_db) << std::endl;
        FreeLibrary(m_sqlite3_dll);
        return;
    }
    std::cout << "Opened database successfully" << std::endl;


    sql1 = "CREATE TABLE IF NOT EXISTS USER("
      "LOGIN          TEXT PRIMARY KEY  NOT NULL,"
      "NAME           TEXT              NOT NULL,"
      "PASSWORD_HASH  TEXT              NOT NULL,"
      "LAST_SEEN      TEXT              NOT NULL,"
      "IS_HAS_PHOTO   INTEGER           NOT NULL,"
      "PHOTO_PATH     TEXT              NOT NULL,"
      "PHOTO_SIZE     INTEGER           NOT NULL,"
      "FRIENDS_LOGINS TEXT);";


    rc = sqlite3_exec(m_db, sql1, 0, 0, &zErrMsg);
    if (rc != 0) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        FreeLibrary(m_sqlite3_dll);
        return;
    }
    else {
        std::cout << "Table USER created successfully" << std::endl;
    }


    const char* sql2;
    sql2 = "CREATE TABLE IF NOT EXISTS COLLECTED_PACKETS("
        "LOGIN          TEXT              NOT NULL,"
        "PACKET         TEXT              NOT NULL);";

    rc = sqlite3_exec(m_db, sql2, 0, 0, &zErrMsg);
    if (rc != 0) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        FreeLibrary(m_sqlite3_dll);
        return;
    }
    else {
        std::cout << "Table COLLECTED_PACKETS created successfully" << std::endl;
    }
}

void Database::addUser(const std::string& login, const std::string& name, const std::string& passwordHash) {
    const char* sql = "INSERT INTO USER (LOGIN, NAME, PASSWORD_HASH, LAST_SEEN, IS_HAS_PHOTO, PHOTO_PATH, PHOTO_SIZE, FRIENDS_LOGINS) "
        "VALUES (?, ?, ?, ?, 0, '', 0, '');";

    sqlite3_stmt* stmt = nullptr;;
    int rc;

    rc = sqlite3_prepare_v2(m_db, sql, -1, (void**)&stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return;
    }

    std::string time = getCurrentDateTime().c_str();
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, passwordHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, time.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(m_db) << std::endl;
    }
    else {
        std::cout << "User  added successfully" << std::endl;
    }

    sqlite3_finalize(stmt);
}

User* Database::getUser(const std::string& login, std::shared_ptr<asio::ip::tcp::socket> acceptSocket) {
    sqlite3_stmt* stmt = nullptr;

    std::string sql = "SELECT LOGIN, NAME, PASSWORD_HASH, LAST_SEEN, IS_HAS_PHOTO, PHOTO_PATH, PHOTO_SIZE, FRIENDS_LOGINS FROM USER WHERE LOGIN = ?";

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, (void**)&stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return nullptr;
    }

    rc = sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC); 
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to bind parameter: " << sqlite3_errmsg(m_db) << std::endl;
        sqlite3_finalize(stmt);
        return nullptr;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::string lastSeen = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        bool isHasPhoto = sqlite3_column_int(stmt, 4) != 0; 

        std::string photoPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        int photoSize = sqlite3_column_int(stmt, 6);
        Photo photo(photoPath, photoSize);

        std::string friendsLoginsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        std::vector<std::string> vecUserFriendsLogins = stringToFriends(friendsLoginsStr);

        User* user = nullptr;
        if (isHasPhoto) {
            user = new User(login, passwordHash, name, isHasPhoto, photo, acceptSocket);
        }
        else {
            user = new User(login, passwordHash, name, isHasPhoto, Photo(), acceptSocket);
        }

        sqlite3_finalize(stmt);
        return user;
    }

    else {
        std::cout << "No user found with login: " << login << std::endl;
        sqlite3_finalize(stmt);
        return nullptr;
    }
}

std::vector<std::string> Database::getUsersStatusesVec(const std::vector<std::string>& loginsVec, const std::unordered_map<std::string, User*>& mapOnlineUsers) {
    std::vector<std::string> statuses;
    for (const auto& login : loginsVec) {

        auto it = mapOnlineUsers.find(login);
        if (it != mapOnlineUsers.end()) {
            statuses.push_back("online"); 
            continue;
        }

        std::string statusFromDb = getUser(login)->getLastSeen(); 
        if (!statusFromDb.empty()) {
            statuses.push_back(statusFromDb);
        }
        else {
            statuses.push_back("offline"); 
            std::cout << "User not found: " << login << std::endl; 
        }
    }
    return statuses;
}

bool Database::checkPassword(const std::string& login, const std::string& password) {
    if (!m_db) {
        std::cerr << "db not initialized (err: from check password func)!" << std::endl;
        return false;
    }

    char* zErrMsg = 0;
    int rc;
    std::string sql;

    std::string escapedLogin;
    for (char c : login) {
        if (c == '\'') {
            escapedLogin += "''";
        }
        else {
            escapedLogin += c;
        }
    }

    sql = "SELECT PASSWORD FROM USER WHERE LOGIN = '" + escapedLogin + "';";

    std::string storedHashedPassword;

    auto passwordCallback = [](void* data, int argc, char** argv, char** azColName) -> int {
        if (argc > 0 && argv[0]) {
            *reinterpret_cast<std::string*>(data) = argv[0];
        }
        return 0;
        };

    rc = sqlite3_exec(m_db, sql.c_str(), passwordCallback, &storedHashedPassword, &zErrMsg);
    if (rc != 0) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        FreeLibrary(m_sqlite3_dll);
        sqlite3_free((void*)zErrMsg);
        return false;
    }

    if (storedHashedPassword.empty()) {
        return false;
    }

    return verifyPassword(password, storedHashedPassword);
}

void Database::collect(const std::string& login, const std::string& packet) {
    const char* sql = "INSERT INTO COLLECTED_PACKETS (LOGIN, PACKET) "
        "VALUES (?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc;

    rc = sqlite3_prepare_v2(m_db, sql, -1, (void**)&stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, packet.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(m_db) << std::endl;
    }
    else {
        std::cout << "User  added successfully" << std::endl;
    }

    sqlite3_finalize(stmt);
}

std::vector<std::string> Database::getCollected(const std::string& login) {
    const char* sql = "SELECT PACKET FROM COLLECTED_PACKETS WHERE LOGIN = ?;";
    sqlite3_stmt* stmt = nullptr;
    int rc;
    std::vector<std::string> packets;

    rc = sqlite3_prepare_v2(m_db, sql, -1, (void**)&stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return packets; // ���������� ������ ������ � ������ ������
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* packet = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (packet) {
            packets.emplace_back(packet);
        }
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);

    return packets; 
}

void Database::updateUser(const std::string& login, const std::string& name, const std::string& password, bool isHasPhoto, Photo photo) {
    const char* sql = "UPDATE USER SET NAME = ?, PASSWORD_HASH = ?, IS_HAS_PHOTO = ?, PHOTO_PATH = ?, PHOTO_SIZE = ? WHERE LOGIN = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc;

    // ���������� SQL-�������
    rc = sqlite3_prepare_v2(m_db, sql, -1, (void**)&stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return;
    }

    // �������� ����������
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashPassword(password).c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, isHasPhoto ? 1 : 0); // ����������� bool � int
    sqlite3_bind_text(stmt, 4, isHasPhoto ? photo.getPhotoPath().c_str() : "", -1, SQLITE_STATIC); // ���� � ����
    sqlite3_bind_int(stmt, 5, isHasPhoto ? photo.getSize() : 0); // ������ ����
    sqlite3_bind_text(stmt, 6, login.c_str(), -1, SQLITE_STATIC); // ����� ��� ������ ������������

    // ���������� �������
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Execution failed: " << sqlite3_errmsg(m_db) << std::endl;
    }
    else {
        std::cout << "User  updated successfully" << std::endl;
    }

    sqlite3_finalize(stmt);
}


// Convert a byte array to a hex string
std::string Database::byteArrayToHexString(const BYTE* data, size_t dataLength) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < dataLength; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

// New generateSalt function (generates 16 bytes = 32 hex characters)
std::string Database::generateSalt() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(0, 255);

    BYTE saltBytes[16]; // 16 bytes = 128 bits
    for (int i = 0; i < 16; ++i) {
        saltBytes[i] = static_cast<BYTE>(distribution(gen));
    }

    return byteArrayToHexString(saltBytes, 16); // Convert byte array to hex string
}

// Function to extract the salt from the stored hash (assuming salt is stored as the first 32 characters = 16 bytes hex)
std::string Database::extractSalt(const std::string& storedHash) {
    if (storedHash.length() < 33) {
        throw std::runtime_error("Invalid stored hash format (salt missing)");
    }
    return storedHash.substr(0, 32); // Salt is the first 32 characters
}

// Function to extract the hash from the stored hash (assuming salt is stored as the first 32 characters, and then ':' and then hash)
std::string Database::extractHash(const std::string& storedHash) {

    size_t delimiterPos = storedHash.find(':');
    if (delimiterPos == std::string::npos) {
        throw std::runtime_error("Invalid stored hash format (delimiter missing)");
    }
    if (delimiterPos + 1 >= storedHash.length()) {
        throw std::runtime_error("Invalid stored hash format (hash missing)");
    }
    return storedHash.substr(delimiterPos + 1); // Hash is after the salt and ':'
}


// Rewritten bcryptHash function (still doesn't truly use bcrypt but uses SHA256 with a salt)
std::string Database::bcryptHash(const std::string& password, const std::string& salt) {
    // **WARNING: This is NOT true bcrypt!  It's SHA256 with a salt.**
    // Proper bcrypt implementation requires using a dedicated bcrypt library.

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    DWORD hashLength = 0;
    BYTE* hash = nullptr;

    std::string saltedPassword = salt + password; // Salt before the password

    // Open the algorithm provider for SHA256
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);


    // Create the hash object
    status = BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0); // No salt to BCryptCreateHash


    // Hash the data (salted password)
    status = BCryptHashData(hHash, (PBYTE)saltedPassword.c_str(), saltedPassword.length(), 0);


    // Get the hash length
    status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&hashLength, sizeof(DWORD), nullptr, 0);


    // Allocate memory for the hash
    hash = new BYTE[hashLength];

    // Finish the hash
    status = BCryptFinishHash(hHash, hash, hashLength, 0);


    // Convert hash to hex string
    std::string hashedPassword = byteArrayToHexString(hash, hashLength);

    // Clean up
    delete[] hash;
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return hashedPassword;
}

// New functions:
std::string Database::hashPassword(const std::string& password) {
    std::string salt = generateSalt();
    std::string hashedPassword = bcryptHash(password, salt); // Still SHA256!
    return salt + ":" + hashedPassword; // Store salt:hash
}

bool Database::verifyPassword(const std::string& password, const std::string& storedHash) {

    try {
        std::string salt = extractSalt(storedHash);
        std::string storedHashedPassword = extractHash(storedHash);
        std::string hashedPassword = bcryptHash(password, salt);

        return hashedPassword == storedHashedPassword;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Error verifying password: " << e.what() << std::endl;
        return false; // Or handle the error differently
    }
}


std::string Database::friendsToString(const std::vector<std::string>& friends) {
    std::stringstream ss;
    for (size_t i = 0; i < friends.size(); ++i) {
        ss << friends[i];
        if (i < friends.size() - 1) {
            ss << ",";
        }
    }
    return ss.str();
}

std::vector<std::string> Database::stringToFriends(const std::string& friendsString) {
    std::vector<std::string> friends;
    std::stringstream ss(friendsString);
    std::string friendLogin;
    while (std::getline(ss, friendLogin, ',')) {
        friends.push_back(friendLogin);
    }
    return friends;
}


std::string Database::getCurrentDateTime() {
    std::time_t now = std::time(0);
    std::tm* ltm = std::localtime(&now);

    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << ltm->tm_year + 1900 << "-"
        << std::setw(2) << std::setfill('0') << ltm->tm_mon + 1 << "-"
        << std::setw(2) << std::setfill('0') << ltm->tm_mday << " "
        << std::setw(2) << std::setfill('0') << ltm->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << ltm->tm_min << ":"
        << std::setw(2) << std::setfill('0') << ltm->tm_sec;

    return "last seen: " + ss.str();
}