#include "RegisterDialog.h"
#include "ssl.h"

namespace gui {

RegisterDialog::RegisterDialog(wxWindow* parent, const std::string& selected_sever) :
    wxDialog(parent, wxID_ANY, "Регистрация пользователя", wxDefaultPosition, wxSize(400,300)),
    server_(selected_sever){
    
    ConstructInterface();

    network_client_ = std::make_unique<Client>();
    network_client_->set_handler([this](const std::string& msg) {
        CallAfter([this, msg] {
            HandleNetworkMessage(msg);
            });
        });
    network_client_->start(server_, CLIENT_FIRST_PORT, CLIENT_SECOND_PORT);
}

void RegisterDialog::ConstructInterface() {
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    //Установка шрифта по умолчанию
    this->SetFont(DEFAULT_FONT);

    // Добавляем информацию о сервере
    wxStaticText* server_info = new wxStaticText(
        panel, wxID_ANY,
        wxString::Format("Регистрация на сервере: %s", server_)
    );
    main_sizer->Add(server_info, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    //поля ввода
    wxFlexGridSizer* input_sizer = new wxFlexGridSizer(3, 2, 10, 15);
    input_sizer->AddGrowableCol(1, 1);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, "Логин"), 0, wxALIGN_CENTER_VERTICAL);
    username_field_ = new wxTextCtrl(panel, wxID_ANY);
    input_sizer->Add(username_field_, 1, wxEXPAND);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, "Пароль:"), 0, wxALIGN_CENTER_VERTICAL);
    password_field_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    input_sizer->Add(password_field_, 1, wxEXPAND);

    input_sizer->Add(new wxStaticText(panel, wxID_ANY, "Подтвердите:"), 0, wxALIGN_CENTER_VERTICAL);
    confirm_password_field_ = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    input_sizer->Add(confirm_password_field_, 1, wxEXPAND);

    main_sizer->Add(input_sizer, 0, wxALL | wxEXPAND, 20);

    //кнопки
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer();

    cancel_button_ = new wxButton(panel, wxID_CANCEL, "Отмена");
    register_button_ = new wxButton(panel, wxID_CANCEL, "Зарегистрировать");

    button_sizer->Add(register_button_, 0, wxRIGHT, 10);
    button_sizer->Add(cancel_button_, 0);


    main_sizer->Add(button_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);
    panel->SetSizer(main_sizer);

    //bind event
    register_button_->Bind(wxEVT_BUTTON, &RegisterDialog::OnRegister, this);
    cancel_button_->Bind(wxEVT_BUTTON, &RegisterDialog::onCancel, this);

    Center();
}

void RegisterDialog::OnRegister(wxCommandEvent& event) {
    if (username_field_->IsEmpty() || password_field_->IsEmpty() || confirm_password_field_->IsEmpty()) {
        wxMessageBox("Все поля должны быть заполнены", "Ошибка", wxICON_ERROR);
        return;
    }

    if (password_field_->GetValue() != confirm_password_field_->GetValue()) {
        wxMessageBox("Пароли не совпадают", "Ошибка", wxICON_ERROR);
        return;
    }

    wxString username = GetUsername();
    std::string sended_username = "@" + username.ToStdString();
    wxString password_hash = ssl::HashPassword(GetPassword().ToStdString());

    network_client_->register_user(sended_username, password_hash.ToStdString());
    std::cout << "[REGISTER] send user: " << username << " and pass: " << password_hash << '\n';
}

void RegisterDialog::onCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void RegisterDialog::HandleNetworkMessage(const std::string& json_msg) {
    std::cout << "[REGISTER] Received: " << json_msg << "\n";

    try {
        auto j = nlohmann::json::parse(json_msg);

        // Добавляем вывод для диагностики
        std::cout << "[Network] Handling message: " << j.dump() << "\n";

        if (!j.contains("type")) {
            throw std::runtime_error("Missing 'type' field");
        }

        int type = j["type"];
        IncomingMessage msg;
        msg.timestamp = std::chrono::system_clock::now();

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
        else if (type == REGISTER) {

            if (!j.contains("answer") || !j.contains("what")) {
                throw std::runtime_error("Missing fields in server response");
            }

            std::string answer = j["answer"].get<std::string>();
            std::string text_register_ = j["what"].get<std::string>();

            if (answer == "OK") {
                wxMessageBox("Регистрация прошла успешно!", "Успех", wxICON_INFORMATION);
                EndModal(wxID_OK);
            }
            else if(answer == "err" && j.contains("reason") && j["reason"] == "login exists") {
                wxMessageBox("Пользователь с таким именем уже существует", "Ошибка", wxICON_ERROR);
            }
            else {
                wxMessageBox("Невозможно подключиться к серверу", "Ошибка", wxICON_ERROR);
            }
        }
        else {
            std::cerr << "[Network] Unknown message type: " << type << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Network] Error handling message: " << e.what() << "\n";
    }
}



}//end namespace gui