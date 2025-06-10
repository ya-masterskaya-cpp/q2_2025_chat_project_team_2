#pragma once
#include <functional>
#include <memory>
#include "add_struct.h"
#include "config.h"

namespace client {

class ChatClient {
public:
    using MessageHandler = std::function<void(const IncomingMessage&)>;

    ChatClient(const std::string& server,
        const std::string& username,
        const std::string& token);

    void SetMessageHandler(MessageHandler handler);
    void SendMessageToServer(const OutgoingMessage& msg);
    void CreateRoom(const std::string& room_name);
    void ChangeUsername(const std::string& new_username);
    void LeaveRoom(const std::string& room_name);
    void JoinRoom(const std::string& room_name);
    void StartPrivateChat(const std::string& username);

    const std::string& GetUsername() const { return username_; }

private:
    std::string server_;
    std::string username_;
    std::string token_;
    MessageHandler message_handler_;
};


}//end namespace client