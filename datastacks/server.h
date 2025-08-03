#pragma once

/* Class for connection between the client and the server */
/* Using winsocket, Linux sockets will be implemented soon */


#include <string>
#include <iostream>
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <functional>
#include <vector>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

#define DATASTACKS_RECVBUFFERLEN 1024

namespace DataStacks {
	std::string sha256(const std::string& str);
	class Client {
	private:
		SOCKET m_socket = INVALID_SOCKET;
		uint64_t m_commandsExecuted = 0;
	public:
		Client(SOCKET sock) : m_socket(sock) {};

		SOCKET   GetSocket() const { return this->m_socket; } 
		uint64_t GetCommandsExecuted() const { return this->m_commandsExecuted; }
		void     IncrementCommandsExecuted() { this->m_commandsExecuted++; }
	public:
		bool authorized = false;
	};
	class Server {
	private:
		bool m_initialized = false;
		std::vector<Client> m_clients = {};
		std::function<void(std::string, Client)> m_on_message_;
		std::function<void(int)> m_on_newclient_;

		SOCKET m_serverSocket = INVALID_SOCKET;
	public:
		Server() { m_initialized = false; }
		Server(const char* port);
		~Server();

		void Run();
		void SendString(std::string msg, SOCKET client);
		void DisconnectClient(SOCKET sock);
		std::vector<Client> GetClients() const { return this->m_clients; }
		void SetOnMessageHandler(std::function<void(std::string, Client)> func) { this->m_on_message_ = func; }
		void SetOnNewClientHandler(std::function<void(int)> func) { this->m_on_newclient_ = func; }
	};
}