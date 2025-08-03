#include "app.h"

DataStacks::App::App(const char* port, std::string password) : m_server(port) {
    this->m_passwordHash = sha256(password);
	this->m_server.SetOnMessageHandler([this](std::string str, const Client& client) {
		this->HandleClientMessage(client, str);
		});
    this->m_server.SetOnNewClientHandler([this](int index) {

        std::vector<Client> clients = this->m_server.GetClients();
        SOCKET clientSocket = clients[index].GetSocket();

        char recvBuffer[DATASTACKS_RECVBUFFERLEN];
        int recvResult = recv(clientSocket, recvBuffer, DATASTACKS_RECVBUFFERLEN, 0);
        if (recvResult > DATASTACKS_RECVBUFFERLEN) {
                this->m_server.SendString("ERROR", clientSocket);
        }

         recvBuffer[recvResult] = '\0';
        if (recvResult > 0) {
            // Success
            // Parse json
            try {
                json j = json::parse(recvBuffer);
                if (!j.contains("password")) {
                    // Screw you, no password
                    this->m_server.DisconnectClient(clientSocket);
                    return;
                }

                std::string password_unhashed = j["password"];
                if (sha256(password_unhashed) == this->m_passwordHash) {
                    // Pass
                    clients[index].authorized = true;
                    this->m_server.SendString("OK", clientSocket);
                }
            }
            catch (...) {
                this->m_server.DisconnectClient(clientSocket);
            }
        }

        else if (recvResult == 0) {
                // Disconnect
                spdlog::info("Client disconnected! (in app.cpp)");
        }

        else {
                int err = WSAGetLastError();

                if (err == WSAECONNRESET) {
                    // Manual disconnect
                    closesocket(clientSocket);
                    spdlog::info("Client manually disconnected!");
                }

                else {
                    spdlog::error("recv() FAIL! Error: {}", err);
                }
        }
        
        });
}

void DataStacks::App::Run() {
	this->m_server.Run();
}

// real simple parsing
std::vector<std::string> DataStacks::App::ParseStringTemplate(const std::string& str)
{
    std::vector<std::string> result;
    std::string latest;
    bool in_quotes = false;

    for (const char c : str) {
        if (c == '\"') {
            in_quotes = !in_quotes;  
            continue;
        }
        if (c == ' ' && !in_quotes) {
            if (!latest.empty()) {
                result.push_back(latest);
                latest.clear();
            }
            continue;
        }
        latest += c;
    }

    if (!latest.empty()) {
        result.push_back(latest);
    }

    return result;
}

void DataStacks::App::HandleGet(std::vector<std::string> parts, Client sock) {
    const std::string key = parts[1];
    auto it = this->data.find(key);
    if (it == this->data.end()) {
        this->m_server.SendString("NULL", sock.GetSocket());
        return;
    }
    if (it->second.ttl != std::chrono::seconds(0)) {
        long long unix_timestamp = TimepointToUnix(it->second.when_set);
        auto now = std::chrono::system_clock::now();
        long long now_unix = TimepointToUnix(now);

        // Check if ttl has passed
        bool expired = ((now_unix - unix_timestamp) > it->second.ttl.count());
        if (expired) {
            this->m_server.SendString("NULL", sock.GetSocket());
            return;
        }
    }
    if (it->second.IsString()) {
        std::string to_send = "\"" + *(it->second.string_val) + "\"";
        this->m_server.SendString(to_send, sock.GetSocket());
    }

    else {
        std::vector<std::string> vector = *(it->second.array_val);
        std::string to_send = "";
        for (const std::string& str : vector) {
            to_send += "\"" + str + "\" ";
        }
        to_send.pop_back(); // remove the little space at the end
        this->m_server.SendString(to_send, sock.GetSocket());
    }

    return;
}

void DataStacks::App::HandleSet(std::vector<std::string> parts, Client sock)
{
    const std::string key = parts[1];
    bool is_array = false;
    bool is_string = false;
    // Example string set:
    // SET key "abc"
    // Example array set:
    // SET key "abc" "abc"
    // We need to check if parts.size() == 3
    if (parts.size() == 3) {
        is_string = true;
        const std::string val = parts[2];
        DataStacks::DataStruct datastruct = { std::nullopt, val, std::chrono::seconds(0),std::chrono::system_clock::now() };
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
    }
    else {
        is_array = true;
        std::vector<std::string> result;
        // The values are at 2, 3, 4....
        for (int i = 2; i < parts.size(); ++i) {
            result.push_back(parts[i]);
        }
        DataStacks::DataStruct datastruct = { result, std::nullopt, std::chrono::seconds(0), std::chrono::system_clock::now()};
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
    }
}

void DataStacks::App::HandleSetEx(std::vector<std::string> parts, Client sock)
{
    // SETEX key seconds value
    const std::string key = parts[1];
    const std::string seconds_str = parts[2];
    bool is_array = false;
    bool is_string = false;

    int seconds_int;
    try {
        seconds_int = std::stoi(seconds_str);
    }
    catch (...) {
        this->m_server.SendString("ERROR", sock.GetSocket());
        return;
    }

    if (parts.size() == 3) {
        is_string = true;
        const std::string val = parts[2];
        DataStacks::DataStruct datastruct = { std::nullopt, val, std::chrono::seconds(seconds_int),std::chrono::system_clock::now() };
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
    }
    else {
        is_array = true;
        std::vector<std::string> result;

        for (int i = 3; i < parts.size(); ++i) {
            result.push_back(parts[i]);
        }
        DataStacks::DataStruct datastruct = { result, std::nullopt, std::chrono::seconds(seconds_int), std::chrono::system_clock::now() };
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
    }
    std::println("Set for {} seconds", seconds_int);
}

void DataStacks::App::HandlePushback(std::vector<std::string> parts, Client sock)
{
    // PUSHBACK key "abc"
    auto key = parts[1];
    const int start_index = 2;
    auto it = this->data.find(key);

    if (it == this->data.end()) {
        std::vector<std::string> to_add = {};
        for (size_t i = start_index; i < parts.size(); i++) {
            to_add.push_back(parts[i]);
        }

        DataStacks::DataStruct datastruct{ to_add, std::nullopt, std::chrono::seconds(0), std::chrono::system_clock::now()};
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
        return;
    }
    if (it->second.IsArray()) {
        it->second.array_val->push_back(parts[2]);
        this->m_server.SendString("OK", sock.GetSocket());
        return;
    }
    else this->m_server.SendString("ERROR", sock.GetSocket());
}

void DataStacks::App::HandlePushfront(std::vector<std::string> parts, Client sock)
{
    auto key = parts[1];
    // PUSHFRONT key "abc"
    const int start_index = 2;
    auto it = this->data.find(key);
    if (it == this->data.end()) {
        std::vector<std::string> to_add = {};
        for (size_t i = start_index; i < parts.size(); i++) {
            to_add.push_back(parts[i]);
        }
        DataStacks::DataStruct datastruct{ to_add, std::nullopt, std::chrono::seconds(0), std::chrono::system_clock::now() };
        this->data[key] = datastruct;
        this->m_server.SendString("OK", sock.GetSocket());
        return;
    }

    if (it->second.IsArray()) {
        it->second.array_val->insert(it->second.array_val->begin(), parts[2]);
        this->m_server.SendString("OK", sock.GetSocket());
        return;
    }
    else this->m_server.SendString("ERROR", sock.GetSocket());
}

void DataStacks::App::HandleDelete(std::vector<std::string> parts, Client sock)
{
    // DEL key
    const std::string key = parts[1];
    auto it = this->data.find(key);
    if (it == this->data.end()) {
        // ??
        // maybe ok
        this->m_server.SendString("OK", sock.GetSocket());
        return;
    }

    this->data.erase(it);
    this->m_server.SendString("OK", sock.GetSocket());
    return;
}

void DataStacks::App::HandleClientMessage(Client sock, std::string msg) {
    std::println("[client] > {}", msg);
    auto parts = ParseStringTemplate(msg);
    
    if (parts.size() < 2) {
        this->m_server.SendString("ERROR", sock.GetSocket());
    }

    if (parts[0] == "GET") {
        this->HandleGet(parts, sock);
    }
    else if (parts[0] == "SET") {
        this->HandleSet(parts, sock);
    }
    else if (parts[0] == "SETEX") {
        if (parts.size() < 3) {
            this->m_server.SendString("ERROR", sock.GetSocket());
            return;
        }
        this->HandleSetEx(parts, sock);
    }
    else if (parts[0] == "PUSHBACK") {
        this->HandlePushback(parts, sock);
    }
    else if (parts[0] == "PUSHFRONT") {
        this->HandlePushfront(parts, sock);
    }
    else if (parts[0] == "DEL") {
        this->HandleDelete(parts, sock);
    }
    else if (parts[0] == "PING") {
        // we received the message, so our server is running
        this->m_server.SendString("PONG", sock.GetSocket());
    }
    else if (parts[0] == "DROPALL") {
        for (const auto& [key, value] : this->data) {
            this->data.erase(key);
        }
        this->m_server.SendString("OK", sock.GetSocket());
    }
}