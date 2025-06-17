#include "ChatClient.h"

namespace client {

    ChatClient::ChatClient(const std::string& server,
        const std::string& username,
        const std::string& password) :
        server_(server), username_(username){

        //ТК у нас было подключение по 1 порту, а в реальности 2,
        //то пока сделаем так, потом изменим
        size_t pos = server.find(':');
        std::string host = (pos != std::string::npos) ? server.substr(0, pos) : server;
        std::string port = (pos != std::string::npos) ? server.substr(pos + 1) : "51001";

        std::cout << "[ChatClient] Parsed server: host=" << host << ", port=" << port << "\n";

        network_client_ = std::make_unique<Client>();

        network_client_->set_handler([this](const std::string& json_msg) {
            HandleNetworkMessage(json_msg);
            });

        // Подключение с использованием правильных портов
        try {
            // Порт 9002 - для исходящих сообщений (sender)
            // Порт 9003 - для входящих сообщений (getter)
            network_client_->start(host, "9003", "9002");
        }
        catch (const std::exception& e) {
            std::cerr << "[Network] Connection error: " << e.what() << "\n";
            throw;
        }

        // Регистрация пользователя
        network_client_->register_user(username, password);

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

void ChatClient::SendMessageToServer(const OutgoingMessage& msg) {
    network_client_->send_message(msg.room, msg.text);
}

void ChatClient::CreateRoom(const std::string& room_name) {
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

void ChatClient::HandleNetworkMessage(const std::string& json_msg) {
    
    std::cout << "[Message] Handling message: " << json_msg << "\n";
    try {
        auto j = nlohmann::json::parse(json_msg);

        // Добавляем вывод для диагностики
        std::cout << "[Network] Handling message: " << j.dump() << "\n";

        if (!j.contains("type")) {
            throw std::runtime_error("Missing 'type' field");
        }

        int type = j["type"];
        IncomingMessage msg;
        msg.timestamp = std::chrono::system_clock::now();

        // Обработка приветственного сообщения
        if (type == GENERAL) {
            // Используем комнату из сообщения
            msg.room = j.value("room", MAIN_ROOM_NAME); // Основное исправление

            if (j.contains("content") && j.contains("room") && j.contains("user")) {
                msg.sender = j["user"].get<std::string>();
                msg.room = j["room"].get<std::string>();
                msg.text = j["content"].get<std::string>();      
            }
            else {
                throw std::runtime_error("Invalid GENERAL message format");
            }

            if (message_handler_) {
                message_handler_(msg);
            }
        }
        // Обработка ответов сервера
        else if (type == LOGIN || type == CHANGE_NAME) {

            msg.sender = SYSTEM_SENDER_NAME;
            msg.room = MAIN_ROOM_NAME;

            if (!j.contains("answer") || !j.contains("what")) {
                throw std::runtime_error("Missing fields in server response");
            }

            std::string answer = j["answer"].get<std::string>();
            std::string what = j["what"].get<std::string>();

            switch (type) {
            case LOGIN:
                msg.text = (answer == "OK")
                    ? "Успешный вход в систему"
                    : "Ошибка авторизации: " + what;
                break;

            case CHANGE_NAME:
                if (answer == "OK") {
                    username_ = what;
                    msg.text = "Имя изменено на: " + username_;
                }
                else {
                    msg.text = "Ошибка смены имени: " + what;
                }
                break;
            }

            if (message_handler_) {
                message_handler_(msg);
            }
        }
        else if (type == ENTER_ROOM) {
            std::cout << "[ENTER ROOM result]: " << j["answer"] << '\n';
            if (room_enter_handler_) {
                bool success;
                std::string message = j["what"].get<std::string>();
                if (j["answer"] == "OK") {
                    success = true;
                }
                else {
                    success = false;
                }
                room_enter_handler_(success, message);
            }
        }
        else if (type == LEAVE_ROOM) {
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
        else if (type == CREATE_ROOM) {
            std::cout << "[ROOM result]: " << j["answer"] << '\n';
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
        else if (type == ASK_ROOMS) {
            std::cout << "[ROOMS parse for]: " << j << '\n';
            if (room_list_handler_ && j.contains("rooms")) {
                std::vector<std::string> rooms;
                for (const auto& room : j["rooms"]) {
                    rooms.push_back(room);
                }
                room_list_handler_(rooms);
            }
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
        error_msg.text = "Ошибка: " + std::string(e.what());
        error_msg.timestamp = std::chrono::system_clock::now();

        if (message_handler_) {
            message_handler_(error_msg);
        }
    }
}

}//end namespace client