#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <fstream>
#include <algorithm>
#include "LoginDialog.h"

namespace gui {

LoginDialog::LoginDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, wxString::FromUTF8("Авторизация на сервере"), wxDefaultPosition, wxSize(700, 400)) {
    ConstructInterface();
    LoadConfigData();
}

void LoginDialog::ConstructInterface() {
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);

    //Установка шрифта по умолчанию
    this->SetFont(DEFAULT_FONT);
    

    //левая часть - поля авторизации
    wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    //поля ввода
    wxFlexGridSizer* input_sizer = new wxFlexGridSizer(2, 2, 10, 15);
    input_sizer->AddGrowableCol(1, 1);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8("Логин:")), 0, wxALIGN_CENTER_VERTICAL);
    username_field_ = new wxTextCtrl(panel, wxID_ANY);
    input_sizer->Add(username_field_, 1, wxEXPAND);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8("Пароль:")), 0, wxALIGN_CENTER_VERTICAL);
    password_field_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    input_sizer->Add(password_field_, 1, wxEXPAND);

    left_sizer->Add(input_sizer, 0, wxLEFT | wxBOTTOM, 20);

    //Чекбокс и кнопки
    remember_check_ = new wxCheckBox(panel, wxID_ANY, wxString::FromUTF8("Запомни меня"));
    left_sizer->Add(remember_check_, 0, wxLEFT | wxBOTTOM, 20);

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer();

    cancel_button_ = new wxButton(panel, wxID_CANCEL, wxString::FromUTF8("Отмена"));
    login_button_ = new wxButton(panel, wxID_OK, wxString::FromUTF8("Войти"));
    register_button_ = new wxButton(panel, wxID_ANY, wxString::FromUTF8("Регистрация"));

    button_sizer->Add(login_button_, 0, wxRIGHT, 10);
    button_sizer->Add(register_button_, 0, wxRIGHT, 10);
    button_sizer->Add(cancel_button_, 0);

    left_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    main_sizer->Add(left_sizer, 1, wxEXPAND);

    //правая часть - список серверов
    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);
    right_sizer->Add(new wxStaticText(panel, wxID_ANY, wxString::FromUTF8("Доступные сервера")), 0, wxTOP | wxLEFT | wxRIGHT, 10);

    //список серверов
    server_list_ = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(250, 200));
    right_sizer->Add(server_list_, 1, wxEXPAND | wxALL, 10);

    //Контекстное меню на списке серверов
    server_list_->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent& event) {
        wxMenu menu;
        menu.Append(wxID_ADD, wxString::FromUTF8("Добавить сервер"));
        menu.Append(wxID_DELETE, wxString::FromUTF8("Удалить сервер"));

        menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
            switch (e.GetId()) {
            case wxID_ADD: OnAddServer(e); break;
            case wxID_DELETE: OnDeleteServer(e); break;
            }
            });

        server_list_->PopupMenu(&menu);
        });

    //кнопки управления списком серверов
    wxBoxSizer* server_button_sizer = new wxBoxSizer(wxHORIZONTAL);

    add_button_ = new wxButton(panel, wxID_ANY, wxString::FromUTF8("Добавить"));
    delete_button_ = new wxButton(panel, wxID_ANY, wxString::FromUTF8("Удалить"));

    server_button_sizer->Add(add_button_, 1, wxRIGHT, 5);
    server_button_sizer->Add(delete_button_, 1, wxRIGHT, 5);

    // Добавляем подсказки
    add_button_->SetToolTip(wxString::FromUTF8("Добавить новый сервер вручную"));
    delete_button_->SetToolTip(wxString::FromUTF8("Удалить выбранный сервер"));

    right_sizer->Add(server_button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    main_sizer->Add(right_sizer, 1, wxEXPAND | wxALL, 10);

    panel->SetSizer(main_sizer);

    // Загрузка иконки приложения
    wxIcon appIcon;
#if defined(__WXMSW__)
    appIcon = wxIcon("APP_ICON", wxBITMAP_TYPE_ICO_RESOURCE);
#elif defined(__WXGTK__)
    appIcon = wxIcon(wxString("resources/icon.png"), wxBITMAP_TYPE_PNG);
#elif defined(__WXOSX__)
    appIcon = wxIcon(wxString("resources/icon.icns"), wxBITMAP_TYPE_ICON);
#endif

    if (appIcon.IsOk()) {
        SetIcon(appIcon);
    }

    //биндинг событий
    login_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnLogin, this);
    cancel_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnCancel, this);
    register_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnRegister, this);
    add_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnAddServer, this);
    delete_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnDeleteServer, this);

    //закрыть приложение если окно авторизации закрыто
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        EndModal(wxID_CANCEL);
        });

    Center();
}

void LoginDialog::OnLogin(wxCommandEvent& event) {
    if (username_field_->IsEmpty() || password_field_->IsEmpty() || server_list_->GetSelection() == wxNOT_FOUND) {
        CreateErrorBox("Заполните все поля и выберите сервер");
        return;
    }

    //работа с "запомнеными" данными
    m_remembered_ = RememberMe();
    m_remembered_username_ = m_remembered_ ? GetUsername() : "";

    // Сохраняем конфиг (серверы + пользовательские данные)
    SaveConfigData();

    //пробуем авторизоваться
    std::string username = GetUsername().ToStdString();
    std::string sended_username = "@" + username;
    std::string password = GetPassword().ToStdString();
    std::string hash = ssl::HashPassword(password);

    ConnetToSelectedServer();
    network_client_->check_login(sended_username, hash);
}

void LoginDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void LoginDialog::OnRegister(wxCommandEvent& event) {
    // Проверяем выбран ли сервер
    if (server_list_->GetSelection() == wxNOT_FOUND) {
        CreateErrorBox("Сначала выберите сервер для регистрации");
        return;
    }

    // Получаем выбранный сервер
    wxString selected_server = server_list_->GetStringSelection();

    RegisterDialog dlg(this, selected_server.ToStdString());
    if (dlg.ShowModal() == wxID_OK) {
        username_field_->SetValue(dlg.GetUsername());
        password_field_->SetValue("");
        password_field_->SetFocus();
    }
}

void LoginDialog::OnClose(wxCloseEvent& event) {
    EndModal(wxID_CANCEL);
}

void LoginDialog::OnAddServer(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this,
        wxString::FromUTF8("Введите адрес сервера в формате IP\nНапример: 192.168.1.11"),
        wxString::FromUTF8("Добавление сервера"));

    if (dlg.ShowModal() == wxID_OK) {
        wxString server = dlg.GetValue();
        
        //проверка формата
        if (ValidateServerFormat(server)) {
            if (server_list_->FindString(server) == wxNOT_FOUND) {
                server_list_->Append(server);
                SaveConfigData();
            }
            else {
                CreateInfoBox("Этот сервер уже существует в списке!");
                return;
            }
        } else {
            CreateErrorBox("Некорректный формат сервера!\n"
                "Используйте формат: XXX.XXX.XXX.XXX\n"
                "Где XXX - число от 0 до 255");
        }
    }
}

void LoginDialog::OnDeleteServer(wxCommandEvent& event) {
    int selected = server_list_->GetSelection();
    if (selected == wxNOT_FOUND) {
        CreateErrorBox("Выберите сервер для удаления");
        return;
    }

    wxString server = server_list_->GetString(selected);
    wxMessageDialog confirm_dlg(this,
        wxString::Format(wxString::FromUTF8("Удалить сервер %s из списка?"), server),
        wxString::FromUTF8("Подтвердить удаление"),
        wxYES_NO | wxICON_QUESTION
    );

    // Проверка, что это не последний сервер
    if (server_list_->GetCount() <= 1) {
        CreateErrorBox("Нельзя удалить последний сервер из списка");
        return;
    }

    if (confirm_dlg.ShowModal() == wxID_YES) {
        server_list_->Delete(selected);

        //выбираем следующий или предыдущий сервер
        int new_count = server_list_->GetCount();
        if (new_count > 0) {
            if (selected < new_count) {
                server_list_->SetSelection(selected);
            } else {
                server_list_->SetSelection(new_count - 1);
            }
        }
        SaveConfigData();
    }
}

bool LoginDialog::ValidateServerFormat(const wxString& server) {
    wxStringTokenizer tokenizer(server, ".");
    if (tokenizer.CountTokens() != 4) return false;

    while (tokenizer.HasMoreTokens()) {
        long num;
        wxString token = tokenizer.GetNextToken();

        if (!token.ToLong(&num)) return false;
        if (num < 0 || num > 255) return false;
        if (token.length() > 1 && token[0] == '0') return false;
    }
    return true;
}

void LoginDialog::LoadConfigData() {
    // Загрузка серверов
    for (const auto& server : config_manager_.GetServers()) {
        server_list_->Append(server);
    }

    // Настройка UI на основе конфига
    if (config_manager_.GetRememberMe()) {
        username_field_->SetValue(config_manager_.GetRememberedUsername());
        remember_check_->SetValue(true);
    }

    // Выбор первого сервера
    if (server_list_->GetCount() > 0) {
        server_list_->SetSelection(0);
    }
}

void LoginDialog::SaveConfigData() {
    // Сохранение серверов
    std::vector<wxString> servers;
    for (unsigned i = 0; i < server_list_->GetCount(); i++) {
        servers.push_back(server_list_->GetString(i));
    }
    config_manager_.SetServers(servers);

    // Сохранение настроек пользователя
    config_manager_.SetRememberMe(RememberMe());
    if (RememberMe()) {
        config_manager_.SetRememberedUsername(GetUsername());
    }

    config_manager_.Save();
}

void LoginDialog::ConnetToSelectedServer() {
    wxString selected_server = server_list_->GetStringSelection();
    if (!network_client_ || selected_server != last_connected_server_) {
        std::cerr << "Connect to " << selected_server << '\n';
        network_client_ = std::make_unique<Client>();
        network_client_->set_handler([this](const std::string& msg) {
            CallAfter([this, msg] {
                HandleNetworkMessage(msg);
                });
            });
        network_client_->start(selected_server.ToStdString(), CLIENT_FIRST_PORT, CLIENT_SECOND_PORT);
    }
    last_connected_server_ = selected_server;
}

bool LoginDialog::HandleNetworkMessage(const std::string& json_msg) {
    std::cout << "[Authorize] Received: " << json_msg << "\n";

    try {
        auto j = nlohmann::json::parse(json_msg);

        // Добавляем вывод для диагностики
        std::cout << "[Network] Handling message: " << j.dump() << "\n";

        if (!j.contains("type")) {
            throw std::runtime_error("Missing 'type' field");
        }

        int type = j["type"];

        // Обработка приветственного сообщения
        if (type == GENERAL) {
            if (j.contains("content") && !j.contains("room") && !j.contains("user")) {
                std::cout << "[Message] Handling message: " << j["content"].get<std::string>() << "\n";
            }
            else {
                throw std::runtime_error("Invalid GENERAL message format");
            }
        }
        // Обработка ответов сервера
        else if (type == CHECK_LOGIN) {

            if (!j.contains("answer") || !j.contains("what")) {
                throw std::runtime_error("Missing fields in server response");
            }

            std::string answer = j["answer"].get<std::string>();
            std::string text_authirize = j["what"].get<std::string>();

            if (answer == "OK") {
                EndModal(wxID_OK);
            }
            else if (answer == "err" && j.contains("reason") && j["reason"] == "user exists") {
                CreateErrorBox("Такой пользователь уже авторизирован в системе");
            }
            else if (answer == "err" && j.contains("reason") && j["reason"] == "wrong login or password") {
                CreateErrorBox("Неверный логин или пароль");
            }
            else {
                CreateErrorBox("Невозможно подключиться к серверу");
            }
        }
        else {
            std::cerr << "[Network] Unknown message type: " << type << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Network] Error handling message: " << e.what() << "\n";
    }
    return true;
}


}//end namespace gui