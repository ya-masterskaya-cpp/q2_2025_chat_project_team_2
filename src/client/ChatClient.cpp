#include "ChatClient.h"

namespace client {

    ChatClient::ChatClient(const std::string& server) :
        server_(server){
        network_client_ = std::make_unique<Client>();

        network_client_->set_handler([this](const std::string& json_msg) {
            HandleNetworkMessage(json_msg);
            });

        // Подключение с использованием правильных портов
        try {
            network_client_->start(server_, CLIENT_FIRST_PORT, CLIENT_SECOND_PORT);
        }
        catch (const std::exception& e) {
            std::cerr << "[Network] Connection error: " << e.what() << "\n";
            throw;
        }
    }

void ChatClient::SetLoginHandler(LoginHandler handler) {
    std::cout << "[ChatClient] Setting login handler\n";
    login_handler_ = std::move(handler);
}

void ChatClient::SetMessageHandler(MessageHandler handler) {
    std::cout << "[ChatClient] Setting message handler\n";
    message_handler_ = std::move(handler);
}

void ChatClient::SetRoomListHandler(RoomListHandler handler) {
    std::cout << "[ChatClient] Setting room list handler\n";
    room_list_handler_ = std::move(handler);
}

void ChatClient::SetRoomCreateHandler(RoomCreateHandler handler) {
    std::cout << "[ChatClient] Setting room create handler\n";
    room_create_handler_ = std::move(handler);
}

void ChatClient::SetRoomEnterHandler(RoomEnterHandler handler) {
    std::cout << "[ChatClient] Setting room enter handler\n";
    room_enter_handler_ = std::move(handler);
}
void ChatClient::SetRoomExitHandler(RoomExitHandler handler) {
    std::cout << "[ChatClient] Setting room leave handler\n";
    room_exit_handler_ = std::move(handler);
}

void ChatClient::SetChangeNameHandler(ChangeNameHandler handler) {
    std::cout << "[ChatClient] Setting change name handler\n";
    change_name_handler_ = std::move(handler);
}

void ChatClient::SetRoomUsersHandler(RoomUsersHandler handler) {
    std::cout << "[ChatClient] Setting room users handler\n";
    room_users_handler_ = std::move(handler);
}

void ChatClient::SetOtherUserNewNameHandler(OtherUserNewNameHandler handler) {
    std::cout << "[ChatClient] Setting Other User NewName handler\n";
    other_user_newname_handler_ = std::move(handler);
}


void ChatClient::SendMessageToServer(const OutgoingMessage& msg) {
    network_client_->send_message(msg.room, msg.text);
}

void ChatClient::CreateRoom(const std::string& room_name) {
    std::cout << "SEND REQUEST TO CREATE ROOM " << room_name << '\n';
    network_client_->create_room(room_name);
}

void ChatClient::ChangeUsername(const std::string& new_username) {
    network_client_->change_name(new_username);
}

void ChatClient::LeaveRoom(const std::string& room_name) {
    network_client_->leave_room(room_name);
}

void ChatClient::JoinRoom(const std::string& room_name) {
    network_client_->enter_room(room_name);
}

void ChatClient::RequestRoomList() {
    network_client_->ask_rooms();
}

void ChatClient::RequestUsersForRoom(const std::string& room_name) {
    std::cout << "SEND TO SERVER USER FOR ROOM: " << room_name << '\n';
    network_client_->ask_users(room_name);
}

void ChatClient::RegisterUser(const std::string& user, const std::string& password) {
    network_client_->register_user(user, password);
}

void ChatClient::LoginUser(const std::string& user, const std::string& password) {
    network_client_->login_user(user, password);
}

void ChatClient::Logout() {
    network_client_->leave_chat();
}

void ChatClient::HandleNetworkMessage(const std::string& json_msg) {
    
    std::cout << "[Message] Handling message: " << json_msg << "\n";
    try {
        json j = nlohmann::json::parse(json_msg);
        
        // Добавляем вывод для диагностики
        std::cout << "[Network] Handling message: " << j.dump() << "\n";

        if (!j.contains("type")) {
            throw std::runtime_error("Missing 'type' field");
        }

        int type = j["type"];

        // Обработка ответов сервера
        if (type == GENERAL) { //101
            HandleGeneralMessage(j);
        }
        else if (type == LOGIN) { //111
            HandleLoginMessage(j);
        }
        else if (type == CHANGE_NAME) { //112
            HandleChangeNameMessage(j);
        }
        else if (type == ENTER_ROOM) { //114
            HandleEnterRoomMessage(j);
        }
        else if (type == LEAVE_ROOM) { //116
            HandleLeaveRoomMessage(j);
        }
        else if (type == CREATE_ROOM) { //113
            HandleCreateRoomMessage(j);
        }
        else if (type == ASK_ROOMS) { //115
            HandleAskRoomsMessage(j);
        }
        else if (type == ASK_USERS) { //121
            HandleAskUsersMessage(j);
        }
        else if (type == INFO_NAME) { //102
            HandleInfoNewNameMessage(j);
        }
        else {
            std::cerr << "[Network] Unknown message type: " << type << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Network] Error handling message: " << e.what() << "\n";

        IncomingMessage error_msg;
        error_msg.sender = SYSTEM_SENDER_NAME;
        error_msg.room = MAIN_ROOM_NAME;
        error_msg.text = wxString::FromUTF8("Ошибка: ") + std::string(e.what());
        error_msg.timestamp = std::chrono::system_clock::now();

        if (message_handler_) {
            message_handler_(error_msg);
        }
    }
}

void ChatClient::HandleGeneralMessage(const json& j) {
    IncomingMessage msg;
    //msg.timestamp = std::chrono::system_clock::now();

    if (j.contains("content") && j.contains("room") && j.contains("user")) {
        msg.sender = j["user"].get<std::string>();
        msg.room = j["room"].get<std::string>();
        msg.text = j["content"].get<std::string>();

        int64_t time_ns = j["server_timestamp"].get<uint64_t>();
        msg.timestamp = ConvertToTimePoint(time_ns);
        //std::chrono::time_point t = ConvertToTimePoint(time_ns);
        //IncomingMessage temp;
        //temp.timestamp = t;
        //std::cout << "TIME = " << j["server_timestamp"].get<uint64_t>() <<
        //    " and time = " << temp.formatted_time() << '\n';
    }
    else if (j.contains("content") && !j.contains("room") && !j.contains("user")) {
        std::cout << "[Message] Handling message: " << j["content"].get<std::string>() << "\n";
    }
    else {
        throw std::runtime_error("Invalid GENERAL message format");
    }

    if (message_handler_) {
        message_handler_(msg);
    }
}

void ChatClient::HandleLoginMessage(const json& j) {
    std::cout << "[LOGIN result]: " << j["answer"] << '\n';
    IncomingMessage msg;
    if (message_handler_) {
        std::string message = j["what"].get<std::string>();
        std::string username = j["name"].get<std::string>();
        if (j["answer"] == "OK" && j.contains("name")) {
            msg.room = MAIN_ROOM_NAME;
            msg.sender = SYSTEM_SENDER_NAME;
            msg.text = username + ", добро пожаловать на сервер " + server_;
            msg.timestamp = std::chrono::system_clock::now();
        }
        message_handler_(msg);

        if (login_handler_ && j.contains("name")) {
            login_handler_(username);
        }
    }
}

void ChatClient::HandleChangeNameMessage(const json& j) {
    std::cout << "[CHANGE_NAME result]: " << j["answer"] << '\n';
    if (change_name_handler_) {
        bool success;
        std::string message = j["what"].get<std::string>();
        if (j["answer"] == "OK") {
            success = true;
        }
        else {
            success = false;
        }
        change_name_handler_(success, message);
    }
}

void ChatClient::HandleEnterRoomMessage(const json& j) {
    std::cout << "[ENTER ROOM result]: " << j["answer"] << '\n';
    if (room_enter_handler_) {
        bool success;

        std::string message = j["what"].get<std::string>();
        std::cout << "FOR ROOM: " << message << '\n';
        if (j["answer"] == "OK") {
            success = true;
        }
        else if (j["answer"] != "OK" && j.contains("reason") &&
            j["reason"].get<std::string>() == "user already entered the room") {
            success = true;
        }
        else {
            success = false;
        }
        room_enter_handler_(success, message);
    }
}

void ChatClient::HandleLeaveRoomMessage(const json& j) {
    std::cout << "[LEAVE ROOM result]: " << j["answer"] << '\n';
    if (room_enter_handler_) {
        bool success;
        std::string message = j["what"].get<std::string>();
        if (j["answer"] == "OK") {
            success = true;
        }
        else {
            success = false;
        }
        room_exit_handler_(success, message);
    }
}

void ChatClient::HandleCreateRoomMessage(const json& j) {
    std::cout << "[CREATE ROOM result]: " << j["answer"] << '\n';
    if (room_create_handler_) {
        bool success;
        std::string message = j["what"].get<std::string>();
        if (j["answer"] == "OK") {
            success = true;
        }
        else {
            success = false;
        }
        room_create_handler_(success, message);
    }
}

void ChatClient::HandleAskRoomsMessage(const json& j) {
    std::cout << "[ASK ROOMS parse for]: " << j << '\n';
    if (room_list_handler_ && j.contains("rooms")) {
        std::set<std::string> rooms;
        for (const auto& room : j["rooms"]) {
            rooms.insert(room);
        }
        room_list_handler_(rooms);
    }
}

void ChatClient::HandleAskUsersMessage(const json& j) {
    std::cout << "[ASK_USERS parse for]: " << j << '\n';
    if (room_users_handler_ && j.contains("users")) {
        std::string room = j["room"].get<std::string>();
        std::set<std::string> users;
        for (const auto& user : j["users"]) {
            users.insert(user);
        }
        room_users_handler_(room, users);
    }
}

void ChatClient::HandleInfoNewNameMessage(const json& j) {
    std::cout << "[INFO_NAME parse for]: " << j << '\n';
    if (room_users_handler_ && j.contains("new_name") && j.contains("old name")) {
        std::string new_name = j["new_name"].get<std::string>();
        std::string old_name = j["old name"].get<std::string>();
        other_user_newname_handler_(old_name, new_name);
    }
}

std::chrono::system_clock::time_point ChatClient::ConvertToTimePoint(uint64_t unix_time_ns) {
    //Длительность в наносекундах
    auto ns_duration = std::chrono::nanoseconds(unix_time_ns);

    //conv in time_point from system_clock
    return std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(ns_duration)
    );
}

}//end namespace client