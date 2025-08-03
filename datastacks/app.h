#pragma once

#include "server.h"
#include <algorithm>
#include <memory>
#include <print>
#include <format>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace DataStacks {

	struct DataStruct {
		std::optional<std::vector<std::string>> array_val;
		std::optional<std::string> string_val;
		std::chrono::seconds ttl = std::chrono::seconds(0);
		std::chrono::system_clock::time_point when_set;

		bool IsArray() {
			if (array_val) return true; else return false;
		}
		bool IsString() {
			// can't just do return string_val; 
			// operator overloaded only for if()
			if (string_val) return true; else return false;
		}
	};
	class App {
	private:
		Server m_server;
		std::string m_passwordHash;
	private:

		long long TimepointToUnix(std::chrono::system_clock::time_point time) {
			long long res = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
			return res;
		}
		void HandleGet(std::vector<std::string> parts, Client sock);
		void HandleSet(std::vector<std::string> parts, Client sock);
		void HandleSetEx(std::vector<std::string> parts, Client sock);
		void HandlePushback(std::vector<std::string> parts, Client sock);
		void HandlePushfront(std::vector<std::string> parts, Client sock);
		void HandleDelete(std::vector<std::string> parts, Client sock);
	public:
		App(const char* port, std::string password);
		~App() = default;
		
		// GET key "abc 123"
		// -> {"GET", "key", "abc 123"}
		std::vector<std::string> ParseStringTemplate(const std::string& str);
		void HandleClientMessage(Client, std::string);

		void Run();
	public:
		std::unordered_map<std::string, DataStruct> data; 
	};
}