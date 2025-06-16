ClientHTTP.h - компилируется с конечным приложением-клиентом

main.cpp - сервер, приложение готово к запуску

Для подключения к серверу, в конечном приложении-клиенте надо выполнить код:

  client = new Client();
  
  client->set_handler([this](const std::string& msg) {
       CallAfter([this, msg]() {
           handle_message(msg);
           });
       });
       
  client->start(host, in_port, out_port);

  void handle_message(const std::string& mes); - метод конечного приложения, который будет вызываться в момент прихода сообщений от сервера. Сообщения приходят в сериализованном формате json
  
std::string host - IP сервера, по умолчанию "127.0.0.1"

std::string in_port - порт сервера для отправки сообщений, по умолчанию "9003"

std::string out_port - порт сервера для отправки сообщений, по умолчанию "9002"

После подключения от сервера должно придти сообщение 

{"content":"hello","type":101}

Дальше - регистрация.

client->register_user("my_login", "my_password");

Сообщения сервера:

{"answer":"OK","type":111,"what":"my_login"} - в случае успеха

{"answer":"err","type":111,"what":"my_login"} - в случае отказа

При успехе можно начинать общение. Первоначальное имя в чате совпадает с логином.

Методы объекта-клиента:

client->change_name("new_name"); - изменить имя

client->сreate_room("new_room"); - создать комнату

client->enter_room("room"); - войти в комнату

client->leave_room("room"); - выйти из комнаты

При успехе или неуспехе сервер посылает сообщение того же вида, что при регистрации, но отличающееся полем type.

client->ask_rooms(); - запросить список комнат

Пример ответа сервера:

{"rooms":["my_name","general","room1"],"type":115}

Каждый участник имеет свою комнату для личных сообщений, "general" - комната для сообщений "всем", в ней по умолчанию присутствуют все участники

send_message("room", "message"); - отправить в комнату сообщение

{"content":"message","room":"general","type":101} - пример сообщения, адресованного в комнату

Список типов сообщений:

const int GENERAL = 101;

const int LOGIN = 111;

const int CHANGE_NAME = 112;

const int CREATE_ROOM = 113;

const int ENTER_ROOM = 114;

const int ASK_ROOMS = 115;

const int LEAVE_ROOM = 116;

Сервер завершает работу по команде 'q'
