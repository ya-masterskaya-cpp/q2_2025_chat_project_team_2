#include "ClientApp.h"

bool ClientApp::OnInit() {

    std::cout << "ClientApp starting...\n";
    try {
        auto* log_dlg = new gui::LoginDialog(nullptr);
        std::cout << "Showing login dialog...\n";
        int result = log_dlg->ShowModal();
        log_dlg->Destroy();

        if (result != wxID_OK) {
            std::cout << "Login canceled\n";
            return false;
        }

        wxString server = log_dlg->GetServer();
        wxString username = "@" + log_dlg->GetUsername();
        wxString password = log_dlg->GetPassword();

        std::string hash = ssl::HashPassword(password.ToStdString());

        std::cout << "Creating ChatClient...\n";
        std::cout << "server: " << server << " user: " << username << " hash: " << hash << '\n';
        auto client = std::make_unique<client::ChatClient>(server.ToStdString());

        std::cout << "Creating MainWindow...\n";
        gui::MainWindow* main_window = new gui::MainWindow(std::move(client), username.ToStdString(), hash);

        std::cout << "Showing MainWindow...\n";
        main_window->Show(true);
        SetTopWindow(main_window);

        std::cout << "Application started successfully\n";
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        wxMessageBox("Ошибка запуска приложения: " + wxString(e.what()),
            "Ошибка", wxICON_ERROR);
        return false;
    }

}

int ClientApp::OnExit() {
    wxApp::OnExit();
    return 0;
}