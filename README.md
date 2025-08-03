# DataStacks

An in-memory database written in C++, built for caching purposes.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Description

DataStacks is a lightweight, in-memory database designed for caching frequently accessed data. It provides fast read and write operations, making it suitable for applications where speed and low latency are critical. The server authenticates with a password, and stores both strings and arrays of strings.

- Repository URL: [https://github.com/psychoartist-cpp/datastacks](https://github.com/psychoartist-cpp/datastacks)
- Default Branch: `main`

## Features and Functionality

- **In-Memory Storage:**  Data is stored in RAM for fast access.
- **Data Types:** Supports storing strings and arrays of strings.
- **TTL (Time-To-Live):**  Keys can be set with an expiration time.
- **Authentication:** Requires a password for client authorization.
- **Basic Operations:**
    - `GET`: Retrieve data by key.
    - `SET`: Set a key-value pair (string or array).
    - `SETEX`: Set a key-value pair with an expiration time.
    - `PUSHBACK`: Append a value to an array associated with a key.
    - `PUSHFRONT`: Prepend a value to an array associated with a key.
    - `DEL`: Delete a key-value pair.
    - `PING`: Check server availability.
    - `DROPALL`: Delete all key-value pairs.
- **Client Authentication:** Clients must authenticate with a password sent as a JSON object upon connection.
- **Error Handling:**  Provides basic error messages to clients.

## Technology Stack

- **C++:** Core language for the database implementation.
- **Winsock2 (Windows Sockets):** Used for network communication.
- **OpenSSL:** Used for SHA256 hashing of passwords.
- **spdlog:** Used for logging.
- **jsoncpp:** Used for parsing the password authentication.

## Prerequisites

- **C++ Compiler:**  A C++ compiler that supports C++17 or later (e.g., g++, MSVC).
- **CMake:** Build system for generating project files.
- **OpenSSL:**  Required for SHA256 hashing. Ensure OpenSSL development libraries are installed.
- **spdlog:** Required for logging. Ensure spdlog is installed.
- **jsoncpp:** Required for handling the password authentication.

### Windows Specific Prerequisites

- **Visual Studio:** Required to build on Windows. Ensure the "Desktop development with C++" workload is installed.
- **vcpkg (Recommended):** A C++ package manager that simplifies installing dependencies on Windows.  You can use it to install OpenSSL, spdlog, and jsoncpp.

## Installation Instructions

1.  **Clone the repository:**

    ```bash
    git clone https://github.com/psychoartist-cpp/datastacks
    cd datastacks
    ```

2.  **Build the project:**

    *   **Windows (using CMake and Visual Studio):**

        ```bash
        mkdir build
        cd build
        cmake -G "Visual Studio 17 2022" -A x64 ..
        cmake --build . --config Release
        ```

        (Replace `"Visual Studio 17 2022"` with your Visual Studio version if necessary.  Ensure the generator architecture `-A x64` matches your system.)

    *   **Linux (using CMake and Make):**

        ```bash
        mkdir build
        cd build
        cmake ..
        make
        ```

3.  **Executable Location:** The built executable (`dataholder.exe` on Windows, or `dataholder` on Linux) will be located in the `build` directory (or a subdirectory within `build` based on your configuration).

## Usage Guide

1.  **Set Environment Variables:**

    Before running DataStacks, set the following environment variables:

    -   `DATASTACKS_PASSWORD`: The password required for client authentication (default: `"123"`).
    -   `DATASTACKS_PORT`: The port the server will listen on (default: `"3008"`).

    Example (Windows):

    ```powershell
    $env:DATASTACKS_PASSWORD="your_strong_password"
    $env:DATASTACKS_PORT="4000"
    ```

    Example (Linux):

    ```bash
    export DATASTACKS_PASSWORD="your_strong_password"
    export DATASTACKS_PORT="4000"
    ```

2.  **Run the Server:**

    Navigate to the directory containing the executable (e.g., `build/Release` on Windows or `build` on Linux) and run the server:

    ```bash
    ./dataholder  # Linux
    .\dataholder.exe # Windows
    ```

3.  **Connect with the `datastacks_shell.py` Client:**

    Run the provided Python script `datastacks_shell.py` to interact with the server:

    ```bash
    python datastacks_shell.py
    ```

    The script will prompt you for the DataStacks host (default: `localhost`) and password (default: `123`).

4.  **Issue Commands:**

    Once connected, you can issue commands to the server.  Examples:

    ```
    [datastacks] > SET mykey "hello world"
    OK
    Elapsed: 0.0001
    [datastacks] > GET mykey
    "hello world"
    Elapsed: 0.0001
    [datastacks] > SETEX anotherkey 10 "short lived data"
    OK
    Elapsed: 0.0001
    [datastacks] > GET anotherkey
    "short lived data"
    Elapsed: 0.0001
    [datastacks] > DEL mykey
    OK
    Elapsed: 0.0001
    [datastacks] > GET mykey
    NULL
    Elapsed: 0.0001
    [datastacks] > PUSHBACK mylist "item1"
    OK
    Elapsed: 0.0001
    [datastacks] > PUSHBACK mylist "item2"
    OK
    Elapsed: 0.0001
    [datastacks] > GET mylist
    "item1" "item2"
    Elapsed: 0.0001
    [datastacks] > PUSHFRONT mylist "item0"
    OK
    Elapsed: 0.0001
    [datastacks] > GET mylist
    "item0" "item1" "item2"
    Elapsed: 0.0001
    [datastacks] > DROPALL
    OK
    Elapsed: 0.0001
    ```

## API Documentation

The `DataStacks::App` class handles the logic for the database. The message handling within `DataStacks::App::HandleClientMessage` processes client commands, parses the input, and calls the appropriate handler function.

The server expects a JSON payload containing the password upon initial connection.
For example: `{"password": "your_password"}`

### Commands:

*   **GET key:** Retrieves the value associated with the given key. Returns `"NULL"` if the key does not exist or has expired (TTL). Returns a string wrapped in quotes, or an array of strings, space separated, each wrapped in quotes.

*   **SET key value:** Sets the value of the given key.  `value` is interpreted as a single string if there are only 3 parts total (`SET`, `key`, `value`), otherwise it's interpreted as an array of strings.  For example: `SET mykey "hello world"`, or `SET mylist "item1" "item2" "item3"`.

*   **SETEX key seconds value:** Sets the value of the given key with an expiration time in seconds. Similar to `SET` for string vs array.  For example: `SETEX mykey 60 "short lived"`, or `SETEX mylist 60 "item1" "item2"`.

*   **PUSHBACK key value:** Appends a value to the array associated with the given key. If the key doesn't exist, it creates a new array with the value. If the key exists, but is not an array, it returns `"ERROR"`.

*   **PUSHFRONT key value:** Prepends a value to the array associated with the given key. If the key doesn't exist, it creates a new array with the value. If the key exists, but is not an array, it returns `"ERROR"`.

*   **DEL key:** Deletes the key-value pair associated with the given key.

*   **PING:**  A simple command to check if the server is running.  The server responds with `"PONG"`.

*   **DROPALL:** Deletes all key-value pairs in the database.

## Contributing Guidelines

Contributions are welcome! To contribute to DataStacks, follow these steps:

1.  Fork the repository.
2.  Create a new branch for your feature or bug fix.
3.  Implement your changes.
4.  Test your changes thoroughly.
5.  Submit a pull request with a clear description of your changes.

Please adhere to the existing coding style and conventions.  Ensure your code is well-documented and includes appropriate unit tests.

## License Information

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact/Support Information

For questions, bug reports, or feature requests, please open an issue on the GitHub repository.
