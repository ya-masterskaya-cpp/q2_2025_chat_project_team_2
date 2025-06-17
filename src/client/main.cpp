#include <iostream>
#include <boost/asio.hpp>
#include "ClientApp.h"

int main(int argc, char* argv[]) {

    // Установка системной локализации
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    
    // Для C++ потоков
    std::locale::global(std::locale("ru_RU.UTF-8"));

    wxApp::SetInstance(new ClientApp());
    wxEntryStart(argc, argv);
    wxTheApp->OnInit();
    wxTheApp->OnRun();
    wxEntryCleanup();
    return 0;
}