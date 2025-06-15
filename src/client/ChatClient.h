#pragma once
#include <functional>
#include <memory>
#include "add_struct.h"
#include "config.h"
#include "../../src/server/ClientHTTP.h"

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
    void RequestRoomList();

    const std::string& GetUsername() const { return username_; }

private:
    std::string server_;
    std::string username_;
    std::string token_;
    std::unique_ptr<Client> network_client_;
    MessageHandler message_handler_;

    void HandleNetworkMessage(const std::string& json_msg);
};


}//end namespace client