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
#include "General.h"
#include "Logger.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

struct MsgQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<nlohmann::json> msg_;
    bool is_online = true;

    MsgQueue& operator=(const MsgQueue & rhs) {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_ = rhs.msg_;
        }
        return *this;
    }

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
            cv_.wait(lk, [&] { return msg_.size() > 0 || !is_online; });
            if (!is_online)  {
                break;
            }
            for (auto& mes : msg_) {
                func(mes);
            }
            msg_.clear();
        }
    }

    void invalidate() {
        {
            std::lock_guard<std::mutex> lk(m_);
            is_online = false;
        }
        cv_.notify_all();
    }
    
    bool is_valid() {
        bool b;
        {
            std::lock_guard<std::mutex> lk(m_);
            b = is_online;
        }
        return b;
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
        logger_.logEvent("Server started");
        std::thread(&Server::shuttle, this).detach();
        std::thread(&Server::client_accept, this).detach();
        std::string s;
        while (true) {
            getline(std::cin, s);
            if (s == "q") {
                break;
            }
        }
        logger_.logEvent("Server stopped");
    }
private:    
    void sender(tcp::socket socket, MsgQueue* session) {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();
        ws.write(asio::buffer(
            MesBuilder(GENERAL).add("content","hello").toString()
        ));
        logger_.logEvent("New client accepted");
        session->wait_on_queue([&](const nlohmann::json& j) {
            ws.write(asio::buffer(j.dump())); });
    }

    void getter(tcp::socket socket, MsgQueue* session) {
        std::string name;
        std::string login;
        try {
            websocket::stream<tcp::socket> ws(std::move(socket));
            ws.accept();
            beast::flat_buffer buffer;
            while (true)
            {
                ws.read(buffer);
                nlohmann::json mes = nlohmann::json::parse(beast::buffers_to_string(buffer.data()));
                int type = mes["type"];  
                switch (type)
                {
                case LOGIN:
                    login = mes["user"];
                    name = login_user(session, mes["user"]);
                    break;
                case CHANGE_NAME:
                    name = change_name(session, login, mes["name"]);
                    break;
                case CREATE_ROOM:
                    create_room(session, mes["room"]);
                    break;
                case ENTER_ROOM:
                    enter_room(session, mes["room"]);
                    break;
                case ASK_ROOMS:
                    ask_rooms(session);
                    break;
                case LEAVE_ROOM:
                    leave_room(session, mes["room"]);
                    break;
                case LEAVE_CHAT:
                    remove_user(session, login);
                    break;
                case GENERAL: 
                    mes["user"] = name;
                    in_msg.add(mes);
                    break;
                default:
                    break;
                }
                buffer.consume(buffer.size()); 
            }
        }
        catch (...) {
            if (session)
            {
                session->invalidate();
            }
            logger_.logError("Client " + name + " disconnected");
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

    nlohmann::json make_err_answer(int type, const std::string& what,
        const std::string& reason = "") {
        return MesBuilder(type).add("what", what).add("answer", "err").add("reason",reason).j;
    }

    std::string login_user(MsgQueue* session, const std::string& login) {
        std::string name = login;
        if (logged_users_.count(login)) {
            std::string name = logged_users_[login];
            change_session(session, name);
            return name;
        }
        else {
            add_user(session, login);
            return login;
        }
    }

    void change_session(MsgQueue* session, const std::string& name) {
        MsgQueue* old_session = *users_[name].begin();
        if (old_session->is_valid()) {
            session->add(make_err_answer(LOGIN, name, USER_EXISTS));
        }
        else {
            *session = *old_session;
            for (auto& u : users_) {
                if (u.second.erase(old_session)) {
                    u.second.insert(session);
                }
            }
            delete old_session;
            session->add(make_ok_answer(LOGIN, name));
            logger_.logEvent("Client " + name + " connected again");
        }
    }

    void add_user(MsgQueue* session, const std::string& login) {
        logged_users_[login] = login;
        users_[login].insert(session);
        users_["general"].insert(session);
        session->add(make_ok_answer(LOGIN, login));
        logger_.logEvent("Client " + login + " logged in");
    }

    std::string change_name(MsgQueue* session, const std::string& login,
        const std::string& new_name) {
        std::string old_name = logged_users_[login];
        if (users_.count(new_name)) {
            session->add(make_err_answer(CHANGE_NAME, new_name, NAME_EXISTS));
        }
        else {
            logged_users_[login] = new_name;
            users_.erase(old_name);
            users_[new_name].insert(session);
            old_name = new_name;
            logger_.logEvent("Client " + login + " changed name to " + new_name);
            session->add(make_ok_answer(CHANGE_NAME, new_name));
        }
        return old_name;
    }

    void ask_rooms(MsgQueue* session) {
        std::vector<std::string> v;
        for (auto& u : users_) {
            v.push_back(u.first);
        }
        session->add(
            MesBuilder(ASK_ROOMS).add("rooms", v).j
        );
    }

    void remove_user(MsgQueue* session, const std::string& login) {
        for (auto& u : users_) {
            u.second.erase(session);
        }
        users_.erase(logged_users_[login]);
        logged_users_.erase(login);
        session->invalidate();
        session = nullptr;
        logger_.logEvent("Client " + login + " left chat");
    }

    void create_room(MsgQueue* session, const std::string& room_name) {
        if (users_.count(room_name)) {
            session->add(make_err_answer(CREATE_ROOM, room_name, ROOM_EXISTS));
        }
        else {
            users_[room_name].insert(session);
            session->add(make_ok_answer(CREATE_ROOM, room_name));
            logger_.logEvent("Room " + room_name + " created");
        }
    }

    void enter_room(MsgQueue* session, const std::string& room_name) {
        if (!users_.count(room_name)) {
            session->add(make_err_answer(ENTER_ROOM, room_name, NO_ROOM));
        }
        else if (!users_[room_name].insert(session).second) {
            session->add(make_err_answer(ENTER_ROOM, room_name, ENTER_TWICE));
        }
        else {
            session->add(make_ok_answer(ENTER_ROOM, room_name));
        }
    }

    void leave_room(MsgQueue* session, const std::string& room_name) {
        if (!users_.count(room_name)) {
            session->add(make_err_answer(LEAVE_ROOM, room_name, NO_ROOM));
        }
        else if (!users_[room_name].erase(session)) {
            session->add(make_err_answer(LEAVE_ROOM, room_name, LEAVE_TWICE));
        }
        else {
            session->add(make_ok_answer(LEAVE_ROOM, room_name));
        }
    }

    asio::io_context ioc;
    MsgQueue in_msg; 
    RoomSessions users_;
    Logger logger_;
    std::unordered_map<std::string, std::string> logged_users_;
};

int main() {
    Server serv;
    serv.run_server();
    return 0;
}
