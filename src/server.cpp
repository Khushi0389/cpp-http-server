#include "server.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <cstring>
#include <sys/socket.h>
#include <zlib.h>

Server::Server(int port) : port(port) {}

void Server::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Listening on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);

        if (client_socket >= 0) {
            std::thread(&Server::handle_client, this, client_socket).detach();
        }
    }
}

void Server::handle_client(int client_socket) {
    bool keep_alive = false;
    do {
        char buffer[8192] = {0};
        int bytes_read = read(client_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) break;

        std::string request(buffer);
        std::istringstream req_stream(request);
        std::string method, path, version, line;
        req_stream >> method >> path >> version;

        std::map<std::string, std::string> headers;
        std::string key, value;
        while (std::getline(req_stream, line) && line != "\r") {
            size_t pos = line.find(":");
            if (pos != std::string::npos) {
                key = line.substr(0, pos);
                value = line.substr(pos + 2); // skip ": "
                headers[key] = value;
            }
        }

        std::string body;
        if (headers.count("Content-Length")) {
            int len = std::stoi(headers["Content-Length"]);
            body.resize(len);
            req_stream.read(&body[0], len);
        }

        keep_alive = headers["Connection"] == "keep-alive";

        std::string response = (method == "GET")
            ? get_http_response(path, headers)
            : (method == "POST" ? get_post_response(body) : "HTTP/1.1 405 Method Not Allowed\r\n\r\n");

        send(client_socket, response.c_str(), response.size(), 0);
    } while (keep_alive);

    close(client_socket);
}

std::string Server::get_http_response(const std::string& path, const std::map<std::string, std::string>& headers) {
    std::string file_path = "./static" + (path == "/" ? "/index.html" : path);
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string body = ss.str();
    std::string encoding;

    if (headers.count("Accept-Encoding") && headers.at("Accept-Encoding").find("gzip") != std::string::npos) {
        encoding = "gzip";
        body = gzip_compress(body);
    }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    if (!encoding.empty())
        response << "Content-Encoding: " << encoding << "\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Connection: keep-alive\r\n\r\n";
    response << body;

    return response.str();
}

std::string Server::get_post_response(const std::string& body) {
    std::ostringstream response;
    std::string html = "<html><body><h2>Received POST Data</h2><pre>" + body + "</pre></body></html>";
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << html.size() << "\r\n\r\n";
    response << html;
    return response.str();
}

std::string Server::gzip_compress(const std::string& input) {
    z_stream zs{};
    deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    zs.next_in = (Bytef*)input.data();
    zs.avail_in = input.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        ret = deflate(&zs, Z_FINISH);
        outstring.append(outbuffer, sizeof(outbuffer) - zs.avail_out);
    } while (ret == Z_OK);

    deflateEnd(&zs);
    return outstring;
}
