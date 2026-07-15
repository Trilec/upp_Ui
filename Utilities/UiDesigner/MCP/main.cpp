#include "UiDesignerMcpServer.h"
#include <iostream>
#include <string>

using namespace Upp;

static bool ReadMessage(std::string& message, bool& framed)
{
    message.clear();
    framed = false;
    std::string line;
    if(!std::getline(std::cin, line))
        return false;
    if(!line.empty() && line.back() == '\r')
        line.pop_back();

    const std::string header = "Content-Length:";
    if(line.rfind(header, 0) != 0) {
        message = line;
        return true;
    }

    framed = true;
    size_t length = 0;
    try {
        length = (size_t)std::stoul(line.substr(header.size()));
    }
    catch(...) {
        return false;
    }
    if(length > 64 * 1024 * 1024)
        return false;
    while(std::getline(std::cin, line)) {
        if(line == "\r" || line.empty())
            break;
    }
    message.resize(length);
    std::cin.read(&message[0], (std::streamsize)length);
    return (size_t)std::cin.gcount() == length;
}

static void WriteMessage(const String& response, bool framed)
{
    if(response.IsEmpty())
        return;
    if(framed)
        std::cout << "Content-Length: " << response.GetCount()
                  << "\r\n\r\n";
    std::cout.write(response.Begin(), response.GetCount());
    if(!framed)
        std::cout << '\n';
    std::cout.flush();
}

CONSOLE_APP_MAIN
{
    UiDesignerMcpServer server;
    std::string message;
    bool framed = false;
    while(ReadMessage(message, framed))
        WriteMessage(server.Handle(String(message.c_str())), framed);
}
