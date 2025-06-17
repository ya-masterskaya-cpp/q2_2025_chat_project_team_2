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
    wxDialog(parent, wxID_ANY, "Авторизация на сервере", wxDefaultPosition, wxSize(700, 400)) {
    ConstructInterface();
    LoadConfig();
}

void LoginDialog::ConstructInterface() {
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxHORIZONTAL);

    //Установка шрифта по умолчанию
    SetFont(DEFAULT_FONT);

    //левая часть - поля авторизации
    wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    //поля ввода
    wxFlexGridSizer* input_sizer = new wxFlexGridSizer(2, 2, 10, 15);
    input_sizer->AddGrowableCol(1, 1);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, "Логин:"), 0, wxALIGN_CENTER_VERTICAL);
    username_field_ = new wxTextCtrl(panel, wxID_ANY);
    input_sizer->Add(username_field_, 1, wxEXPAND);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, "Пароль:"), 0, wxALIGN_CENTER_VERTICAL);
    password_field_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    input_sizer->Add(password_field_, 1, wxEXPAND);

    left_sizer->Add(input_sizer, 0, wxLEFT | wxBOTTOM, 20);

    //Чекбокс и кнопки
    remember_check_ = new wxCheckBox(panel, wxID_ANY, "Запомни меня");
    left_sizer->Add(remember_check_, 0, wxLEFT | wxBOTTOM, 20);

    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer();

    cancel_button_ = new wxButton(panel, wxID_CANCEL, "Отмена");
    login_button_ = new wxButton(panel, wxID_OK, "Войти");
    register_button_ = new wxButton(panel, wxID_ANY, "Регистрация");

    button_sizer->Add(login_button_, 0, wxRIGHT, 10);
    button_sizer->Add(register_button_, 0, wxRIGHT, 10);
    button_sizer->Add(cancel_button_, 0);

    left_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    main_sizer->Add(left_sizer, 1, wxEXPAND);

    //правая часть - список серверов
    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);
    right_sizer->Add(new wxStaticText(panel, wxID_ANY, "Доступные сервера"), 0, wxTOP | wxLEFT | wxRIGHT, 10);

    //список серверов
    server_list_ = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(250, 200));
    right_sizer->Add(server_list_, 1, wxEXPAND | wxALL, 10);

    //Контекстное меню на списке серверов
    server_list_->Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent& event) {
        wxMenu menu;
        menu.Append(wxID_ADD, "Добавить сервер");
        menu.Append(wxID_DELETE, "Удалить сервер");
        menu.AppendSeparator();
        menu.Append(wxID_REFRESH, "Обновить список");

        menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
            switch (e.GetId()) {
            case wxID_ADD: OnAddServer(e); break;
            case wxID_DELETE: OnDeleteServer(e); break;
            case wxID_REFRESH: OnUpdateServers(e); break;
            }
            });

        server_list_->PopupMenu(&menu);
        });

    //кнопки управления списком серверов
    wxBoxSizer* server_button_sizer = new wxBoxSizer(wxHORIZONTAL);

    add_button_ = new wxButton(panel, wxID_ANY, "Добавить");
    delete_button_ = new wxButton(panel, wxID_ANY, "Удалить");
    update_button_ = new wxButton(panel, wxID_ANY, "Обновить");
    update_button_->Disable(); //пока отключаем эту кнопку

    server_button_sizer->Add(add_button_, 1, wxRIGHT, 5);
    server_button_sizer->Add(delete_button_, 1, wxRIGHT, 5);
    server_button_sizer->Add(update_button_, 1);

    // Добавляем подсказки
    add_button_->SetToolTip("Добавить новый сервер вручную");
    delete_button_->SetToolTip("Удалить выбранный сервер");
    update_button_->SetToolTip("Обновить список серверов из сети");

    right_sizer->Add(server_button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    main_sizer->Add(right_sizer, 1, wxEXPAND | wxALL, 10);

    panel->SetSizer(main_sizer);

    //биндинг событий
    login_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnLogin, this);
    cancel_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnCancel, this);
    register_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnRegister, this);
    add_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnAddServer, this);
    delete_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnDeleteServer, this);
    update_button_->Bind(wxEVT_BUTTON, &LoginDialog::OnUpdateServers, this);

    //закрыть приложение если окно авторизации закрыто
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        EndModal(wxID_CANCEL);
        });

    Center();
}

void LoginDialog::OnLogin(wxCommandEvent& event) {
    if (username_field_->IsEmpty() || password_field_->IsEmpty() || server_list_->GetSelection() == wxNOT_FOUND) {
        wxMessageBox("Заполните все поля и выберите сервер", "Ошибка", wxICON_WARNING);
        return;
    }

    //работа с "запомнеными" данными
    m_remembered_ = RememberMe();
    m_remembered_username_ = m_remembered_ ? GetUsername() : "";

    // Сохраняем конфиг (серверы + пользовательские данные)
    SaveConfig();

    //для теста передаем данные в главное окно
    wxString message = wxString::Format(
        "Выбран сервер: %s,\t Логин: %s\t, Хэш пароля: %s",
        GetServer(),GetUsername(),ssl::HashPassword(GetPassword().ToStdString())
    );

    wxMessageBox(message, "Данные авторизации", wxOK | wxICON_INFORMATION);
    EndModal(wxID_OK);
}

void LoginDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void LoginDialog::OnRegister(wxCommandEvent& event) {
    // Проверяем выбран ли сервер
    if (server_list_->GetSelection() == wxNOT_FOUND) {
        wxMessageBox("Сначала выберите сервер для регистрации", "Ошибка", wxICON_WARNING);
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
        "Введите адрес сервера в формате IP:PORT\nНапример: 192.168.1.11:51001",
        "Добавление сервера");

    if (dlg.ShowModal() == wxID_OK) {
        wxString server = dlg.GetValue();
        
        //проверка формата
        if (ValidateServerFormat(server)) {
            if (server_list_->FindString(server) == wxNOT_FOUND) {
                server_list_->Append(server);
                SaveConfig();
            }
            else {
                wxMessageBox("Этот сервер уже существует в списке!", "Предупреждение", wxICON_WARNING);
                return;
            }
        } else {
            wxMessageBox(
                "Некорректный формат сервера!\n"
                "Используйте формат: XXX.XXX.XXX.XXX:PORT\n"
                "Где XXX - число от 0 до 255, PORT - от 1 до 65535",
                "Ошибка", wxICON_ERROR
            );
        }
    }
}

void LoginDialog::OnDeleteServer(wxCommandEvent& event) {
    int selected = server_list_->GetSelection();
    if (selected == wxNOT_FOUND) {
        wxMessageBox("Выберите сервер для удаления", "Ошибка", wxICON_WARNING);
        return;
    }

    wxString server = server_list_->GetString(selected);
    wxMessageDialog confirm_dlg(this,
        wxString::Format("Удалить сервер %s из списка?", server),
        "Подтвердить удаление",
        wxYES_NO | wxICON_QUESTION
    );

    // Проверка, что это не последний сервер
    if (server_list_->GetCount() <= 1) {
        wxMessageBox("Нельзя удалить последний сервер из списка", "Ошибка", wxICON_WARNING);
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
        SaveConfig();
    }

}

bool LoginDialog::ValidateServerFormat(const wxString& server) {
    wxString ip_part;
    unsigned long port;
    int pos = server.Find(':');

    if (pos == wxNOT_FOUND) return false;

    ip_part = server.Left(pos);
    wxString port_str = server.Mid(pos + 1);

    //проверка порта
    if (!port_str.ToULong(&port) || port < 1 || port > 65535) {
        return false;
    }

    //проверка ip
    wxStringTokenizer tokenizer(ip_part, ".");
    if (tokenizer.CountTokens() != 4) return false;

    while (tokenizer.HasMoreTokens()) {
        long num;
        wxString token = tokenizer.GetNextToken();

        //проверка что число
        if (!token.ToLong(&num)) return false;

        //проверка лиапозона
        if (num < 0 || num > 255) return false;

        //проверка ведущего октета
        if (token.length() > 1 && token[0] == '0') return false;
    }
    return true;
}

void LoginDialog::LoadConfig() {
    wxString filename = wxStandardPaths::Get().GetUserConfigDir() + CLIENT_FILE_CONFIG;
    server_list_->Clear();
    m_remembered_ = false;
    m_remembered_username_ = "";

    if (wxFileExists(filename)) {
        std::ifstream file(filename.ToStdString());
        std::string line;
        wxString current_section;

        while (std::getline(file, line)) {
            wxString wline = wxString::FromUTF8(line.c_str());
            wline.Trim().Trim(false);

            if (wline.IsEmpty()) continue;

            // Обработка секций
            if (wline.StartsWith("[") && wline.EndsWith("]")) {
                current_section = wline.Mid(1, wline.Length() - 2).Lower();
                continue;
            }

            // Секция серверов
            if (current_section == "servers") {
                server_list_->Append(wline);
            }
            // Секция пользователя
            else if (current_section == "user") {
                int pos = wline.Find('=');
                if (pos != wxNOT_FOUND) {
                    wxString key = wline.Left(pos).Trim().Trim(false).Lower();
                    wxString value = wline.Mid(pos + 1).Trim().Trim(false);

                    if (key == "remember") {
                        m_remembered_ = (value == "1");
                    }
                    else if (key == "username") {
                        m_remembered_username_ = value;
                    }
                }
            }
        }
    }

    // Если серверов нет - добавляем дефолтный
    if (server_list_->GetCount() == 0) {
        server_list_->Append(DEFAULT_SERVER);
    }

    // Выбираем первый сервер
    if (server_list_->GetCount() > 0) {
        server_list_->SetSelection(0);
    }

    // Применяем запомненные настройки
    if (m_remembered_) {
        username_field_->SetValue(m_remembered_username_);
        remember_check_->SetValue(true);
        password_field_->SetFocus();
    }
}

void LoginDialog::SaveConfig() {
    wxString filename = wxStandardPaths::Get().GetUserConfigDir() + CLIENT_FILE_CONFIG;
    wxFileName fn(filename);

    // Создаем директорию если нужно
    if (!fn.DirExists()) {
        fn.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    std::ofstream file(filename.ToStdString());
    if (!file) return;

    // Секция серверов
    file << "[servers]\n";
    for (unsigned int i = 0; i < server_list_->GetCount(); i++) {
        file << server_list_->GetString(i).ToUTF8().data() << "\n";
    }

    // Секция пользователя
    file << "\n[user]\n";
    UpdateUserSection(file);

}

void LoginDialog::UpdateUserSection(std::ofstream& file) {
    file << "remember=" << (m_remembered_ ? "1" : "0") << "\n";

    if (m_remembered_ && !m_remembered_username_.empty()) {
        file << "username=" << m_remembered_username_.ToUTF8().data() << "\n";
    }
}

void LoginDialog::OnUpdateServers(wxCommandEvent& event) {
    //TODO: реализовать обновление серверов
    wxMessageBox("Функция обновления серверов будет реализована позже",
        "Информация", wxICON_INFORMATION);
}

}//end namespace gui