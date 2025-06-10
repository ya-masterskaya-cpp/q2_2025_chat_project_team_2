#include "ClientApp.h"

bool ClientApp::OnInit() {

    // Создаем диалог авторизации
    auto* log_dlg = new gui::LoginDialog(nullptr);

    // Используем ShowModal() в сочетании с обработчиком закрытия
    int result = log_dlg->ShowModal();
    log_dlg->Destroy(); // Явно уничтожаем диалог

    if (result != wxID_OK) {
        // Если авторизация отменена - завершаем приложение
        return false;
    }

    wxString server = log_dlg->GetServer();
    wxString username = log_dlg->GetUsername();
    wxString password = log_dlg->GetPassword();

    std::string token = ssl::HashPassword(password.ToStdString());

    try {
        auto client = std::make_unique<client::ChatClient>(
            server.ToStdString(),
            username.ToStdString(),
            token
        );

        // Создаем главное окно
        gui::MainWindow* main_window = new gui::MainWindow(std::move(client));
        main_window->Show(true);

        // Устанавливаем главное окно как самое верхноуровневое
        SetTopWindow(main_window);
        return true;
    }
    catch (const std::exception& e) {
        wxMessageBox("Ошибка подключения: " + wxString(e.what()), "Ошибка", wxICON_ERROR);
        return false;
    }
}

int ClientApp::OnExit() {
    wxApp::OnExit();
    return 0;
}