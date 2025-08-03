#include "server.h"

std::string DataStacks::sha256(const std::string & str) {
	EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
	const EVP_MD* md = EVP_sha256();
	unsigned char hash[SHA256_DIGEST_LENGTH];
	unsigned int hash_len;

	EVP_DigestInit_ex(mdctx, md, nullptr);
	EVP_DigestUpdate(mdctx, str.c_str(), str.size());
	EVP_DigestFinal_ex(mdctx, hash, &hash_len);
	EVP_MD_CTX_free(mdctx);

	std::stringstream ss;
	for (unsigned int i = 0; i < hash_len; i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
	}

	return ss.str();
}
DataStacks::Server::Server(const char* port) : m_initialized(false) {
	WSADATA wsaData;
	int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaResult != 0) {
		spdlog::error("WSAStartup failed! Error: {}", wsaResult);
		return;
	}

	struct addrinfo* adrResult = NULL;
	struct addrinfo adrHints;

	ZeroMemory(&adrHints, sizeof(adrHints));
	adrHints.ai_family = AF_INET;
	adrHints.ai_socktype = SOCK_STREAM;
	adrHints.ai_protocol = IPPROTO_TCP;
	adrHints.ai_flags = AI_PASSIVE;

	int resolveResult = getaddrinfo(NULL, port, &adrHints, &adrResult);
	if (resolveResult != 0) {
		spdlog::error("Failed to resolve! Error: {}", resolveResult);
		WSACleanup();
		return;
	}

	this->m_serverSocket = socket(adrResult->ai_family, adrResult->ai_socktype, adrResult->ai_protocol);
	if (this->m_serverSocket == INVALID_SOCKET) {
		spdlog::error("socket() FAILED! Error: {}", WSAGetLastError());
		freeaddrinfo(adrResult);
		WSACleanup();
		return;
	}

	int bindResult = bind(this->m_serverSocket, adrResult->ai_addr, (int)adrResult->ai_addrlen);
	if (bindResult == SOCKET_ERROR) {
		spdlog::error("bind() FAILED! Error: {}", WSAGetLastError());
		freeaddrinfo(adrResult);
		closesocket(this->m_serverSocket);
		WSACleanup();
		return;
	}

	freeaddrinfo(adrResult);
	m_initialized = true;
	spdlog::info("Server initialized successfully!");
}

DataStacks::Server::~Server() {
	if (!m_initialized) return;
	spdlog::info("Beginning shutdown procedure...");
	// Shutdown connection with each client 
	for (const Client& client : m_clients) {
		this->DisconnectClient(client.GetSocket());
	}
	closesocket(this->m_serverSocket);
	WSACleanup();
}

void DataStacks::Server::Run() {
	if (!m_initialized) {
		spdlog::error("Cannot run server - initialization failed!");
		return;
	}

	int listenResult = listen(this->m_serverSocket, SOMAXCONN);
	if (listenResult == SOCKET_ERROR) {
		spdlog::error("Failed to listen! Error: {}", WSAGetLastError());
		return;
	}
	spdlog::info("Listening...");
	while (true) {
		SOCKET clientSocket = accept(this->m_serverSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET) {
			spdlog::warn("Failed to accept client! Error: ", WSAGetLastError());
			continue;
		}
		spdlog::info("Client connected!");
		this->m_clients.push_back(Client(clientSocket));

		if(this->m_on_newclient_) {
			// last element, the one we just pushed is at .size() - 1
			this->m_on_newclient_(this->m_clients.size() - 1);
		}

		std::thread clientThread([this, clientSocket]() {
			char recvBuffer[DATASTACKS_RECVBUFFERLEN];

			while (true) {
				int recvResult = recv(clientSocket, recvBuffer, DATASTACKS_RECVBUFFERLEN, 0);
				if (recvResult > DATASTACKS_RECVBUFFERLEN) {
					this->SendString("ERROR", clientSocket);
					continue;
				}

				recvBuffer[recvResult] = '\0'; // The CIA wants you to keep strings null terminated
				
				if (recvResult > 0) {
					// Success
					if (this->m_on_message_) {
						this->m_on_message_(std::string(recvBuffer), Client(clientSocket));
					}
				}
				
				else if (recvResult == 0) {
					// Disconnect
					spdlog::info("Client disconnected!");
					break;
				}

				else {
					int err = WSAGetLastError();

					if(err == WSAECONNRESET) {
						// Manual disconnect
						closesocket(clientSocket);
						spdlog::info("Client manually disconnected!");
					}

					else {
						spdlog::error("recv() FAIL! Error: {}", err);
					}
				}
			}
			});
		clientThread.detach();
	}
}

void DataStacks::Server::DisconnectClient(SOCKET sock) {
	int shutResult = shutdown(sock, SD_SEND);
	spdlog::info((shutResult == SOCKET_ERROR) ?
		"Failed to disconnect client!" : "Disconnected client from server!");
}

void DataStacks::Server::SendString(std::string msg, SOCKET client) {
	int sendRes = send(client, msg.c_str(), msg.size(), 0);
	if (sendRes == SOCKET_ERROR) {
		spdlog::error("Failed to send! Error: {}", WSAGetLastError());
		return;
	}
}