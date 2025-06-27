#include "ClientApp.h"
#include <wx/fontmap.h>

int main(int argc, char* argv[]) {

    // Установка системной локализации
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    
    // Для C++ потоков
    std::locale::global(std::locale("ru_RU.UTF-8"));

    //for Windows (/SUBSYSTEM:WINDOWS)
    //wxDISABLE_DEBUG_SUPPORT();
    //wxInitializer initializer;
    //if (!initializer.IsOk()) {
    //    std::cerr << "Failed to initialize wxWidgets" << std::endl;
    //    return -1;
    //}

    //// Создание и запуск приложения
    //ClientApp* app = new ClientApp();
    //wxApp::SetInstance(app);
    //return wxEntry(argc, argv);


    //for Консоль (/SUBSYSTEM:CONSOLE)
    wxApp::SetInstance(new ClientApp());
    wxEntryStart(argc, argv);
    wxTheApp->OnInit();
    wxTheApp->OnRun();
    wxEntryCleanup();
    return 0;
}