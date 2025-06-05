#include <sdkddkver.h>
#define BOOST_DISABLE_CURRENT_LOCATION

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include "json.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

const int GENERAL = 101;
const int LOGIN = 111;
const int NAME = 112;

struct MsgQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<nlohmann::json> msg_;

    void add(const nlohmann::json& j)
    {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_.push_back(j);
        }
        cv_.notify_all();
    }
    template<typename F>
    void wait_on_queue(F func)
    {
        std::unique_lock<std::mutex> lk(m_);
        while (true) {
            cv_.wait(lk);
            for (auto& mes : msg_)
            {
                func(mes);
            }
            msg_.clear();
        }
    }
};

const int in_port = 9002;
const int out_port = 9003;
const std::string host = "127.0.0.1";

using UserSessions = std::unordered_map<std::string, MsgQueue*>;
using RoomSessions = std::unordered_map<std::string, std::vector<MsgQueue*>>;

class Server
{
public:
    void run_server()
    {
        std::thread(&Server::shuttle, this).detach();
        std::thread(&Server::client_accept, this).detach();
        std::string s;
        while (true)
        {
            getline(std::cin, s);
            if (s == "q")
            {
                break;
            }
        }
    }
private:    
    void sender(tcp::socket socket, MsgQueue* session) {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();
        nlohmann::json hello;
        hello["type"] = GENERAL;
        hello["content"] = "hello";
        ws.write(asio::buffer(hello.dump()));
        session->wait_on_queue([&](const nlohmann::json& j) {
            ws.write(asio::buffer(j.dump())); });
    }

    void getter(tcp::socket socket, MsgQueue* session) {
        try {
            websocket::stream<tcp::socket> ws(std::move(socket));
            ws.accept();
            beast::flat_buffer buffer;
            /*start cycle*/
            while (true)
            {
                ws.read(buffer);
                nlohmann::json j =
                    nlohmann::json::parse(beast::buffers_to_string(buffer.data()));
                int type = j["type"];
                switch (type)
                {
                case LOGIN:
                    session->add(j);
                    break;
                case NAME:
                    users_[j["name"]] = session;
                    session->add(j);
                    break;
                case GENERAL:                
                    in_msg.add(j);
                    break;
                default:
                    break;
                }
                buffer.consume(buffer.size()); 
            }
        }
        catch (...) {
            std::cout << "Read error: " << std::endl;
        }
    }
    
    void client_accept()
    {
        tcp::acceptor in_acceptor(ioc, tcp::endpoint(tcp::v4(), in_port));
        tcp::acceptor out_acceptor(ioc, tcp::endpoint(tcp::v4(), out_port));
        while (true) {
            MsgQueue* session = new MsgQueue;

            tcp::socket in_socket(ioc);
            in_acceptor.accept(in_socket);
            std::thread(&Server::getter, this, std::move(in_socket), session).detach();
            
            tcp::socket out_socket(ioc);
            out_acceptor.accept(out_socket);
            std::thread(&Server::sender, this, std::move(out_socket), session).detach();
            
        }
    }

    void shuttle()
    {
        in_msg.wait_on_queue([&](const nlohmann::json& j) {

            for (auto& user : users_)
            {
                user.second->add(j);
            }});
            
        return;
    }

    asio::io_context ioc;
    MsgQueue in_msg;
    UserSessions users_;
};

int main() {
    Server serv;
    serv.run_server();
    return 0;
}