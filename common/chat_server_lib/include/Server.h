#pragma once
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
#include <set>

#include "nlohmann/json.hpp"    //добавил
//#include "json.h"
#include <unordered_set>
#include "General.h"
#include "Logger.h"
#include <atomic>               //добавил
#include <csignal>              //добавил
#include "db.hpp"
#include "time_utils.hpp"
#include "common_struct.h"

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
    bool is_online_ = true;
    bool is_valid_ = true;
    std::string login;
    std::string name;

    MsgQueue& operator=(const MsgQueue& rhs) {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_.insert(msg_.end(), rhs.msg_.begin(), rhs.msg_.end());
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
    bool wait_on_queue(F func) {
        std::unique_lock<std::mutex> lk(m_);
        while (true) {
            cv_.wait(lk, [&] { return !msg_.empty() || !is_online_ || !is_valid_; });
            if (!is_online_ || !is_valid_) {
                break;
            }
            for (auto& mes : msg_) {
                func(mes);
            }
            msg_.clear();
        }
        return is_valid_;
    }

    void invalidate() {
        {
            std::lock_guard<std::mutex> lk(m_);
            is_online_ = false;
        }
        cv_.notify_all();
    }

    void finish() {
        {
            std::lock_guard<std::mutex> lk(m_);
            is_valid_ = false;
        }
        cv_.notify_all();
    }
};

const int in_port = 9002;
const int out_port = 9003;
const std::string host = "127.0.0.1";

using UserSessions = std::unordered_map<std::string, MsgQueue*>;
using RoomSessions = std::unordered_map<std::string, std::unordered_set<MsgQueue*>>;

class Server
{
public:
    virtual void run_server(db::DB& data_base);

    //добавил деструктор
    virtual ~Server() { 
        logger_.logEvent("Server shutting down gracefully");
    }


protected:
    void sender(tcp::socket socket, MsgQueue* session);
    void getter(tcp::socket socket, MsgQueue* session);
    virtual void client_accept();
    virtual void shuttle();

    nlohmann::json make_ok_answer(int type, const std::string& what);
    nlohmann::json make_err_answer(int type, const std::string& what, const std::string& reason = "");
    bool check_login(const std::string& login, const std::string& password);
    nlohmann::json register_user(const std::string& login, const std::string& password);
    void change_session(MsgQueue* session, const std::string& login);
    nlohmann::json add_user(MsgQueue* session, const std::string& login);
    nlohmann::json change_name(MsgQueue* session, const std::string& new_name);
    nlohmann::json ask_rooms(MsgQueue* session);
    nlohmann::json ask_users(const std::string& room);
    void remove_user(MsgQueue* session);
    nlohmann::json create_room(MsgQueue* session, const std::string& room_name);
    nlohmann::json enter_room(MsgQueue* session, const std::string& room_name);
    nlohmann::json leave_room(MsgQueue* session, const std::string& room_name);

//private:
    db::DB* db_;
    asio::io_context ioc;
    MsgQueue in_msg;
    RoomSessions users_;
    Logger logger_;
    std::unordered_map<std::string, std::string> logged_users_;
    std::unordered_map<std::string, std::string> users_passwords_;
    asio::signal_set signals_{ ioc, SIGINT, SIGTERM };      //добавил
    std::atomic<bool> is_running_{ true };                  //добавил

    // для тестового сервера
    tcp::acceptor in_acceptor_{ ioc };  // Инициализируем сразу с io_context
    tcp::acceptor out_acceptor_{ ioc }; // Инициализируем сразу с io_context

    // КЭШ: <имя_комнаты, {логин1, логин2, ...}>
    std::unordered_map<std::string, std::unordered_set<std::string>> members_who_wrote_;
    // Мьютекс для защиты members_who_wrote_ 
    std::mutex members_mutex_;
};