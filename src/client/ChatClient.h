#pragma once
#include <functional>
#include <memory>
#include <set>
#include "add_struct.h"
#include "config.h"
#include "ClientHTTP.h"

namespace client {

using json = nlohmann::json;


class ChatClient {
public:
    using LoginHandler = std::function<void(const std::string&)>;
    using MessageHandler = std::function<void(const IncomingMessage&)>;
    using RoomListHandler = std::function<void(const std::set<std::string>&)>;
    using RoomCreateHandler = std::function<void(bool, const std::string&)>;
    using RoomEnterHandler = std::function<void(bool, const std::string&)>;
    using RoomExitHandler = std::function<void(bool, const std::string&)>;
    using ChangeNameHandler = std::function<void(bool, const std::string&)>;
    using RoomUsersHandler = std::function<void(const std::string&, const std::set<std::string>&)>;
    using OtherUserNewNameHandler = std::function<void(const std::string&, const std::string&)>;

    ChatClient(const std::string& server);

    void SetLoginHandler(LoginHandler handler);
    void SetMessageHandler(MessageHandler handler);
    void SetRoomListHandler(RoomListHandler handler);
    void SetRoomCreateHandler(RoomCreateHandler handler);
    void SetRoomEnterHandler(RoomEnterHandler handler);
    void SetRoomExitHandler(RoomExitHandler handler);
    void SetChangeNameHandler(ChangeNameHandler handler);
    void SetRoomUsersHandler(RoomUsersHandler handler);
    void SetOtherUserNewNameHandler(OtherUserNewNameHandler handler);
    void SendMessageToServer(const OutgoingMessage& msg);
    void CreateRoom(const std::string& room_name);
    void ChangeUsername(const std::string& new_username);
    void LeaveRoom(const std::string& room_name);
    void JoinRoom(const std::string& room_name);
    void RequestRoomList();
    void RequestUsersForRoom(const std::string& room_name);

    void RegisterUser(const std::string& user, const std::string& password);
    void LoginUser(const std::string& user, const std::string& password);
    void Logout();

private:
    std::string server_;
    std::unique_ptr<Client> network_client_;
    LoginHandler login_handler_;
    MessageHandler message_handler_;
    RoomListHandler room_list_handler_;
    RoomCreateHandler room_create_handler_;
    RoomEnterHandler room_enter_handler_;
    RoomExitHandler room_exit_handler_; 
    ChangeNameHandler change_name_handler_;
    RoomUsersHandler room_users_handler_;
    OtherUserNewNameHandler other_user_newname_handler_;

    void HandleNetworkMessage(const std::string& json_msg);
    void HandleGeneralMessage(const json& j);
    void HandleLoginMessage(const json& j);
    void HandleChangeNameMessage(const json& j);
    void HandleEnterRoomMessage(const json& j);
    void HandleLeaveRoomMessage(const json& j);
    void HandleCreateRoomMessage(const json& j);
    void HandleAskRoomsMessage(const json& j);
    void HandleAskUsersMessage(const json& j);
    void HandleInfoNewNameMessage(const json& j);


    std::chrono::system_clock::time_point ConvertToTimePoint(uint64_t unix_time_ns);
};


}//end namespace client