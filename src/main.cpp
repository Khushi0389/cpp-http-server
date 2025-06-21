#include "server.hpp"

int main() {
    Server server(8080); //port chosen
    server.start();
    return 0;
}
