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

const std::string host = "127.0.0.1";
const std::string in_port = "9003";
const std::string out_port = "9002";

const int GENERAL = 101;
const int LOGIN = 111;
const int NAME = 112;

struct Mes
{
    std::mutex m_;
    std::condition_variable cv_;
    std::string msg_;
    void add(const std::string& s)
    {
        {
            std::lock_guard<std::mutex> lk(m_);
            msg_ = s;
        }
        cv_.notify_all();
    }

    template<typename F>
    void wait_on_message(F func)
    {
        std::unique_lock<std::mutex> lk(m_);
        while (true) {
            cv_.wait(lk);
            func(msg_);
        }
    }
};

using MessageHandler = std::function<void(const std::string&)>;
class Client
{
public:
    void start()
    {
        std::thread(&Client::sender, this).detach();
        
    }
    void set_handler(MessageHandler mh)
    {
        message_handler = mh;
    }
    void send_message(const std::string& message)
    {
        msg_.add(message);        
    }
private:
    void sender()
    {
        websocket::stream<tcp::socket> ws(ioc);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, out_port);
        try
        {
            asio::connect(ws.next_layer(), results);
            ws.handshake(host, "/");
            std::thread(&Client::getter, this).detach();
            msg_.wait_on_message([&](const std::string& s) {
                ws.write(asio::buffer(s));
                });           
        }
        catch (...)
        {
            message_handler("Error connect");
        }
    }

    void getter()
    {
        websocket::stream<tcp::socket> ws(ioc);
        tcp::resolver resolver(ioc);
        auto const results = resolver.resolve(host, in_port);
        try
        {
            asio::connect(ws.next_layer(), results);
            ws.handshake(host, "/");
            std::string mesg_;
            beast::flat_buffer buffer;            
            while (true)
            {
                ws.read(buffer);
                message_handler(beast::buffers_to_string(buffer.data()));
                buffer.consume(buffer.size());
            }
        }
        catch (...)
        {
            message_handler("Read or connection error");
        }
    }
    asio::io_context ioc;
    Mes msg_;
    bool finish_ = false;
    MessageHandler message_handler;
};

