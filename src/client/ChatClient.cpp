#include "ChatClient.h"

namespace client {

ChatClient::ChatClient(const std::string& server,
                    const std::string& username,
                    const std::string& token) :
server_(server), username_(username), token_(token){}

void ChatClient::SetMessageHandler(MessageHandler handler) {
    message_handler_ = handler;
}

void ChatClient::SendMessageToServer(const OutgoingMessage& msg) {
    //Эмуляция - создаем входящее сообщение из исходящего
    IncomingMessage inc_msg;
    inc_msg.sender = username_;
    inc_msg.room = msg.room;
    inc_msg.text = msg.text;
    inc_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(inc_msg);
    }
}

void ChatClient::CreateRoom(const std::string& room_name) {
    //Эмуляция - сервер всегда подтверждает создание комнаты
    IncomingMessage sys_msg;
    sys_msg.sender = SYSTEM_SENDER_NAME;
    sys_msg.room = room_name;
    sys_msg.text = "Комната '" + room_name + "' создана";
    sys_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(sys_msg);
    }
}

void ChatClient::ChangeUsername(const std::string& new_username) {
    //Эмуляция - сервер подтвердил смену имени
    IncomingMessage sys_msg;
    sys_msg.sender = SYSTEM_SENDER_NAME;
    sys_msg.room = MAIN_ROOM_NAME;
    sys_msg.text = "Имя пользователя изменено на '" + new_username + "'";
    sys_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(sys_msg);
    }

    //обновляем имя в клиенте
    username_ = new_username;
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
    // Эмуляция - сервер подтверждает присоединение
    IncomingMessage sys_msg;
    sys_msg.sender = SYSTEM_SENDER_NAME;
    sys_msg.room = room_name;
    sys_msg.text = "Вы присоединились к комнате '" + room_name + "'";
    sys_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(sys_msg);
    }
}

void ChatClient::StartPrivateChat(const std::string& username) {
    // Эмуляция - сервер подтверждает начало приватного чата
    IncomingMessage sys_msg;
    sys_msg.sender = SYSTEM_SENDER_NAME;
    sys_msg.room = "@" + username;
    sys_msg.text = "Приватный чат с '" + username + "' начат";
    sys_msg.timestamp = std::chrono::system_clock::now();

    if (message_handler_) {
        message_handler_(sys_msg);
    }
}

//void ChatClient::SimulateIncomeMessage() {
//    IncomingMessage msg{ SYSTEM_SENDER_NAME, MAIN_ROOM_NAME, "Welcome to chat!",
//               std::chrono::system_clock::now() };
//    if (message_handler_) {
//        message_handler_(msg);
//    }
//}


}//end namespace client