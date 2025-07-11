/*Вот так вот работает верно, но в некоторых IDE может отображать ошибку:
Вызвано исключение по адресу 0x00007FFE5A5CB7B4 (KernelBase.dll) в server.exe: 0x40010005: Control-C.

Объяснение:
На Windows Ctrl+C вызывает Console Control Event, а не SIGINT как на Linux. 
Boost.Asio ловит SIGINT, но Windows консоль по умолчанию вызывает ExitProcess, 
если ты явно не перехватываешь CTRL_C_EVENT

Если запускаем отдельно или из консоли - все работает
*/
#include "Server.h"

int main() {
   // {
    db::DB data_base;
    data_base.OpenDB();
    Server serv;
    serv.run_server(data_base);

    data_base.CloseDB();
    //}
    return 0;
}



