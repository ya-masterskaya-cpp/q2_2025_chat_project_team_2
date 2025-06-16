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
#include <unordered_set>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

const int GENERAL = 101;
const int LOGIN = 111;
const int CHANGE_NAME = 112;
const int CREATE_ROOM = 113;
const int ENTER_ROOM = 114;
const int ASK_ROOMS = 115;
const int LEAVE_ROOM = 116;

struct MsgQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<nlohmann::json> msg_;
    bool is_online = true;

    void add(const nlohmann::json& j) {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_.push_back(j);
        }
        cv_.notify_all();
    }

    template<typename F>
    void wait_on_queue(F func) {
        std::unique_lock<std::mutex> lk(m_);
        while (true) {
            cv_.wait(lk, [&] { return msg_.size() > 0 && is_online; });
            for (auto& mes : msg_) {
                func(mes);
            }
            msg_.clear();
        }
    }

    void validate(bool b) {
        {
            std::lock_guard<std::mutex> lk(m_);
            is_online = b;
        }
    }
};

struct MesBuilder
{
    MesBuilder(int type) {
        j["type"] = type;
    }
    MesBuilder& add(const std::string& key, const std::string& value) {
        j[key] = value;
        return *this;
    }
    MesBuilder& add(const std::string& key, const std::vector<std::string>& value) {
        j[key] = value;
        return *this;
    }
    std::string toString() {
        return j.dump();
    }
    nlohmann::json j;
};

const int in_port = 9002;
const int out_port = 9003;
const std::string host = "127.0.0.1";

using UserSessions = std::unordered_map<std::string, MsgQueue*>;
using RoomSessions = std::unordered_map<std::string, std::unordered_set<MsgQueue*>>;

class Server
{
public:
    void run_server() {
        std::thread(&Server::shuttle, this).detach();
        std::thread(&Server::client_accept, this).detach();
        std::string s;
        while (true) {
            getline(std::cin, s);
            if (s == "q") {
                break;
            }
        }
    }
private:    
    void sender(tcp::socket socket, MsgQueue* session) {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();
        ws.write(asio::buffer(
            MesBuilder(GENERAL).add("content","hello").toString()
        ));
        session->wait_on_queue([&](const nlohmann::json& j) {
            ws.write(asio::buffer(j.dump())); });
    }

    void getter(tcp::socket socket, MsgQueue* session) {
        try {
            websocket::stream<tcp::socket> ws(std::move(socket));
            ws.accept();
            beast::flat_buffer buffer;
            std::string name;
            std::string s;
            nlohmann::json mes;
            std::vector<std::string> v;
            int type;
            while (true)
            {
                ws.read(buffer);
                mes = nlohmann::json::parse(beast::buffers_to_string(buffer.data()));
                type = mes["type"];  
                switch (type)
                {
                case LOGIN:
                    name = mes["user"];
                    users_[name].insert(session);
                    users_["general"].insert(session);
                    session->add(make_ok_answer(LOGIN,name));
                    break;
                case CHANGE_NAME:
                    s = mes["name"];
                    if (users_.count(s)) {
                        session->add(make_err_answer(CHANGE_NAME, s));
                    }
                    else {                       
                        users_.erase(name);
                        users_[s].insert(session);
                        name = s;
                        session->add(make_ok_answer(CHANGE_NAME, s));
                    }
                    break;
                case CREATE_ROOM:
                    s = mes["room"];
                    if (users_.count(s)) {
                        session->add(make_err_answer(CREATE_ROOM, s));
                    }
                    else {
                        users_[s].insert(session);                        
                        session->add(make_ok_answer(CREATE_ROOM, s));
                    }
                    break;
                case ENTER_ROOM:
                    s = mes["room"];
                    if (!users_.count(s))  {
                        session->add(make_err_answer(ENTER_ROOM, s));
                    }
                    else { 
                        users_[s].insert(session);                        
                        session->add(make_ok_answer(ENTER_ROOM, s));
                    }
                    break;
                case ASK_ROOMS:
                    for (auto& u : users_) {
                        v.push_back(u.first);
                    }
                    session->add(
                        MesBuilder(ASK_ROOMS).add("rooms", v).j
                    );
                    v.clear();
                    break;
                case LEAVE_ROOM:
                    s = mes["room"];
                    if (!users_.count(s)) {
                        session->add(make_err_answer(LEAVE_ROOM, s));
                    }
                    else {
                        users_[s].erase(session);
                        session->add(make_ok_answer(LEAVE_ROOM, s));
                    }
                    break;
                case GENERAL:                
                    in_msg.add(mes);
                    break;
                default:
                    break;
                }
                buffer.consume(buffer.size()); 
            }
        }
        catch (...) {
            session->validate(false);
            std::cout << "Read error" << std::endl;
        }
    }
    
    void client_accept() {
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

    void shuttle() {
        in_msg.wait_on_queue([&](const nlohmann::json& j) {
            for (auto ses : users_[j["room"]]) {
                ses->add(j);
            }});
    }

    nlohmann::json make_ok_answer(int type, const std::string& what) {
        return MesBuilder(type).add("what", what).add("answer", "OK").j;
    }

    nlohmann::json make_err_answer(int type, const std::string& what) {
        return MesBuilder(type).add("what", what).add("answer", "err").j;
    }

    void remove_user(MsgQueue* session) {
        for (auto& u : users_) {
            u.second.erase(session);
            if (u.second.empty()) {
                users_.erase(u.first);
            }
        }
    }

    asio::io_context ioc;
    MsgQueue in_msg; 
    RoomSessions users_;
};

int main() {
    Server serv;
    serv.run_server();
    return 0;
}