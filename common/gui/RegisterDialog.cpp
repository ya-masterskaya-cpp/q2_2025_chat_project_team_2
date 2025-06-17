#include "RegisterDialog.h"
#include "ssl.h"

namespace gui {

RegisterDialog::RegisterDialog(wxWindow* parent, const std::string& selected_sever) :
    wxDialog(parent, wxID_ANY, "Регистрация пользователя", wxDefaultPosition, wxSize(400,300)),
    server_(selected_sever){
    ConstructInterface();
}

void RegisterDialog::ConstructInterface() {
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    //Установка шрифта по умолчанию
    SetFont(DEFAULT_FONT);

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

    //Эмуляция запроса к серверу
    wxString username = GetUsername();
    wxString password_hash = ssl::HashPassword(GetPassword().ToStdString());

    //Эмуляция ответа от сервера
    enum Responce {OK, NAME_ERROR, CONNECT_ERROR};
    Responce server_responce = rand() % 10 < 8 ? OK : (rand() % 2 ? NAME_ERROR : CONNECT_ERROR);

    switch (server_responce)
    {
    case OK:
        wxMessageBox("Регистрация прошла успешно!", "Успех", wxICON_INFORMATION);
        EndModal(wxID_OK);
        break;
    case NAME_ERROR:
        wxMessageBox("Пользователь с таким именем уже существует", "Ошибка", wxICON_ERROR);
        break;
    case CONNECT_ERROR:
        wxMessageBox("Невозможно подключиться к серверу", "Ошибка", wxICON_ERROR);
        break;
    default:
        break;
    }
}

void RegisterDialog::onCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

}//end namespace gui