ClientHTTP.h - компилируется с конечным приложением-клиентом

main.cpp - сервер, приложение готово к запуску

В конечном приложении-клиенте выполнить код:

  client = new Client();
  
  client->set_handler([this](const std::string& msg) {
       CallAfter([this, msg]() {
           handle_message(msg);
           });
       });
       
  client->start(host, in_port, out_port);

  void handle_message(const std::string& mes); - метод конечного приложения, который будет вызываться в момент прихода сообщений от сервера
  
std::string host - IP сервера, по умолчанию "127.0.0.1"

std::string in_port - порт сервера для отправки сообщений, по умолчанию "9003"

std::string out_port - порт сервера для отправки сообщений, по умолчанию "9002"
