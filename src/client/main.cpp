#include "ClientApp.h"

int main(int argc, char* argv[]) {

    // Установка системной локализации
    std::setlocale(LC_ALL, "ru_RU.UTF-8");
    std::locale::global(std::locale("ru_RU.UTF-8"));

    wxDISABLE_DEBUG_SUPPORT();
    return wxEntry(argc, argv);
}