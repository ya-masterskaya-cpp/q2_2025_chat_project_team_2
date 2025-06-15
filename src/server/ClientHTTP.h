#pragma once
#include <sdkddkver.h>
#define BOOST_DISABLE_CURRENT_LOCATION

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <vector>
#include "nlohmann/json.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

const std::string def_host = "127.0.0.1";
const std::string def_in_port = "9003";
const std::string def_out_port = "9002";

const int GENERAL = 101;
const int LOGIN = 111;
const int CHANGE_NAME = 112;
const int CREATE_ROOM = 113;
const int ENTER_ROOM = 114;
const int ASK_ROOMS = 115;

// Логгер для вывода времени
static void log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::cout << "[" << std::put_time(std::localtime(&now_time), "%T")
        << "." << std::setfill('0') << std::setw(3) << now_ms.count()
        << "] [CLIENT] " << message << std::endl;
}

struct MesQueue
{
    std::mutex m_;
    std::condition_variable cv_;
    std::vector<std::string> msg_;
    void add(const std::string& s) {
        {
            std::lock_guard<std::mutex> lk(m_);
            log("Adding message to queue: " + s);
            msg_.push_back(s);
        }
        cv_.notify_all();
    }

    template<typename F>
    void wait_on_message(F func) {
        std::unique_lock<std::mutex> lk(m_);
        log("Starting message processing loop");
        while (true) {
            log("Waiting for messages...");
            cv_.wait(lk, [&] { 
                log("Wakeup check: messages=" + std::to_string(msg_.size()));
                return msg_.size() > 0; 
                } );
            log("Processing " + std::to_string(msg_.size()) + " messages");
            for (auto& mes : msg_) {
                try {
                    func(mes);
                }
                catch (const std::exception& e) {
                    log("Error processing message: " + std::string(e.what()));
                }
            }
            msg_.clear();
        }
        log("Message processing loop ended");
    }
};

struct MesBuilder
{
    MesBuilder(int type) {
        j["type"] = type;
    }
    MesBuilder& add(const std::string key, const std::string value) {
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

using MessageHandler = std::function<void(const std::string&)>;
class Client
{
public:
    void start(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        log("Starting network threads for " + host + ":" + in_port + "/" + out_port);
        std::thread(&Client::sender, this, host, in_port, out_port).detach();
    }

    void set_handler(MessageHandler mh) {
        log("Setting message handler");
        message_handler = mh;
    }

    void send_message(const std::string& room, const std::string& message) {
        auto msg = MesBuilder(GENERAL).add("room", room).add("content", message).toString();
        log("Sending message: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(GENERAL).add("room", room).add("content", message).toString()
        //);
    }

    void register_user(const std::string& user, const std::string& password) {
        auto msg = MesBuilder(LOGIN).add("user", user).add("password", password).toString();
        log("Registering user: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(LOGIN).add("user", user).add("password", password).toString()
        //);
    }

    void change_name(const std::string& name) {
        auto msg = MesBuilder(CHANGE_NAME).add("name", name).toString();
        log("Changing name: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(CHANGE_NAME).add("name", name).toString()
        //);
    }

    void create_room(const std::string& room) {
        auto msg = MesBuilder(CREATE_ROOM).add("room", room).toString();
        log("Creating room: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(CREATE_ROOM).add("room", room).toString()
        //);
    }

    void enter_room(const std::string& room) {
        auto msg = MesBuilder(ENTER_ROOM).add("room", room).toString();
        log("Entering room: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(ENTER_ROOM).add("room", room).toString()
        //);
    }

    void ask_rooms() {
        auto msg = MesBuilder(ASK_ROOMS).toString();
        log("Requesting room list: " + msg);
        mq_.add(msg);
        //mq_.add(
        //    MesBuilder(ASK_ROOMS).toString()
        //);
    }

private:
    void sender(const std::string& host, const std::string& in_port,
        const std::string& out_port) {
        log("Sender thread started");
        try {
            websocket::stream<tcp::socket> ws(ioc);
            tcp::resolver resolver(ioc);

            log("Resolving " + host + ":" + out_port);
            auto const results = resolver.resolve(host, out_port);

            log("Connecting to " + host + ":" + out_port);
            asio::connect(ws.next_layer(), results);

            log("Performing WebSocket handshake");
            ws.handshake(host, "/");

            log("Starting receiver thread");
            std::thread(&Client::getter, this, host, in_port).detach();

            log("Starting message loop");
            mq_.wait_on_message([&](const std::string& s) {
                log("Sending message: " + s);
                try {
                    ws.write(asio::buffer(s));
                }
                catch (const std::exception& e) {
                    log("Write error: " + std::string(e.what()));
                    if (message_handler) {
                        message_handler("{\"type\":0,\"error\":\"Write error: " + std::string(e.what()) + "\"}");
                    }
                }
                });
        }
        catch (const std::exception& e) {
            log("Sender exception: " + std::string(e.what()));
            if (message_handler) {
                message_handler("{\"type\":0,\"error\":\"Connection error: " + std::string(e.what()) + "\"}");
            }
        }
        log("Sender thread ended");
    }

    void getter(const std::string& host, const std::string& in_port) {
        log("Receiver thread started");
        try {
            websocket::stream<tcp::socket> ws(ioc);
            tcp::resolver resolver(ioc);

            log("Resolving " + host + ":" + in_port);
            auto const results = resolver.resolve(host, in_port);

            log("Connecting to " + host + ":" + in_port);
            asio::connect(ws.next_layer(), results);

            log("Performing WebSocket handshake");
            ws.handshake(host, "/");

            beast::flat_buffer buffer;
            while (true) {
                log("Waiting for message...");
                ws.read(buffer);

                auto message = beast::buffers_to_string(buffer.data());
                log("Received message: " + message);

                if (message_handler) {
                    message_handler(message);
                }

                buffer.consume(buffer.size());
            }
        }
        catch (const std::exception& e) {
            log("Receiver exception: " + std::string(e.what()));
            if (message_handler) {
                message_handler("{\"type\":0,\"error\":\"Read error: " + std::string(e.what()) + "\"}");
            }
        }
        log("Receiver thread ended");
    }

    asio::io_context ioc;
    MesQueue mq_;
    bool finish_ = false;
    MessageHandler message_handler;
};

