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
    using RoomListHandler = std::function<void(const std::vector<std::string>&)>;
    using RoomCreateHandler = std::function<void(bool, const std::string&)>;
    using RoomEnterHandler = std::function<void(bool, const std::string&)>;
    using RoomExitHandler = std::function<void(bool, const std::string&)>;

    ChatClient(const std::string& server,
        const std::string& username,
        const std::string& token);

    void SetMessageHandler(MessageHandler handler);
    void SetRoomListHandler(RoomListHandler handler);
    void SetRoomCreateHandler(RoomCreateHandler handler);
    void SetRoomEnterHandler(RoomEnterHandler handler);
    void SetRoomExitHandler(RoomExitHandler handler);
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
    RoomListHandler room_list_handler_;
    RoomCreateHandler room_create_handler_;
    RoomEnterHandler room_enter_handler_;
    RoomExitHandler room_exit_handler_;

    void HandleNetworkMessage(const std::string& json_msg);
};


}//end namespace client