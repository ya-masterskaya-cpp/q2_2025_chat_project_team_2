#include "ClientApp.h"
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <windows.h> // Для работы с консолью Windows
#endif

void ClientApp::OnInitCmdLine(wxCmdLineParser& parser) {
    wxApp::OnInitCmdLine(parser);
    parser.SetDesc([] {
        static wxCmdLineEntryDesc desc[] = {
            { wxCMD_LINE_SWITCH, "c", "console", "run in console mode" },
            { wxCMD_LINE_NONE }
        };
        return desc;
        }());
}

bool ClientApp::OnCmdLineParsed(wxCmdLineParser& parser) {
    console_mode_ = parser.Found("console") || parser.Found("c");
    return wxApp::OnCmdLineParsed(parser);
}

bool ClientApp::OnInit() {

    // Активируем консоль, если запрошен консольный режим
    if (console_mode_) {
#ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            std::ios::sync_with_stdio(true);
            std::cout.clear();
            std::cerr.clear();
        }
#else
        // Для Linux/MacOS просто используем стандартные потоки
        std::ios::sync_with_stdio(true);
#endif
    }

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
    if (console_mode_) {
#ifdef _WIN32
        FreeConsole(); // Закрываем консоль при выходе
#endif
    }
    return wxApp::OnExit();
}

wxIMPLEMENT_APP(ClientApp);