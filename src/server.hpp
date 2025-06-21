#pragma once
#include <string>
#include <map>

class Server {
public:
    Server(int port);
    void start();

private:
    int port;
    int server_fd;
    void handle_client(int client_socket);
    std::string get_http_response(const std::string& path, const std::map<std::string, std::string>& headers);
    std::string get_post_response(const std::string& body);
    std::string gzip_compress(const std::string& input);
};
