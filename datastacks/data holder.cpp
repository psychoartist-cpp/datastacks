#include "app.h"

std::optional<std::string> GetEnvVariable(const std::string_view& varname) {
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, varname.data()) != 0 || buf == nullptr) {
        return std::nullopt;
    }
    std::string result(buf, sz);
    free(buf);
    return result;
}

int main() {
    auto password = GetEnvVariable("DATASTACKS_PASSWORD");
    auto port = GetEnvVariable("DATASTACKS_PORT");
    // Value of password if successfully initialized, else:
    // default password 123
	DataStacks::App app(
        (port) ? port->c_str() : "3008", (password) ? *password : "123");

	app.Run();

	return 0;
}