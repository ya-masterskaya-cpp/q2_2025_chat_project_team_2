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

        //network_client_ = std::make_unique<Client>();
        //network_client_->set_handler([this](const std::string& json_msg) {
        //    HandleNetworkMessage(json_msg);
        //    });

        //// Подключение к серверу
        //network_client_->start(server, "9003", "9002");
        //network_client_->register_user(username, password);
    }

void ChatClient::SetMessageHandler(MessageHandler handler) {
    std::cout << "[ChatClient] Setting message handler\n";
    message_handler_ = handler;
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
    // Эмуляция - сервер подтвердил выход из комнаты
    IncomingMessage sys_msg;
    sys_msg.sender = SYSTEM_SENDER_NAME;
    sys_msg.room = room_name;
    sys_msg.text = "Вы покинули комнату '" + room_name + "'";
    sys_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(sys_msg);
    }
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
            // Для приветственного сообщения сервера
            if (j.contains("content")) {
                msg.sender = SYSTEM_SENDER_NAME;
                msg.room = MAIN_ROOM_NAME;
                msg.text = j["content"].get<std::string>();
            }
            // Для обычных сообщений
            else if (j.contains("sender") && j.contains("room") && j.contains("content")) {
                msg.sender = j["sender"].get<std::string>();
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
        else if (type == LOGIN || type == CHANGE_NAME ||
            type == CREATE_ROOM || type == ENTER_ROOM) {

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

            case CREATE_ROOM:
                msg.text = (answer == "OK")
                    ? "Комната создана: " + what
                    : "Ошибка создания комнаты: " + what;
                break;

            case ENTER_ROOM:
                msg.text = (answer == "OK")
                    ? "Вы вошли в комнату: " + what
                    : "Ошибка входа в комнату: " + what;
                break;
            }

            if (message_handler_) {
                message_handler_(msg);
            }
        }
        else if (type == ASK_ROOMS) {
            // Обработка списка комнат (пока пропустим)
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