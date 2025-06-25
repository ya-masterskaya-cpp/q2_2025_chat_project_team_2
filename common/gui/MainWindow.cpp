#include "MainWindow.h"
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/richtext/richtextformatdlg.h>
#include <wx/richtext/richtextstyles.h>
#include <wx/artprov.h>
#include <wx/settings.h>

namespace gui{

ChatRoomPanel::ChatRoomPanel(wxWindow* parent, const std::string& room_name) :
    wxPanel(parent), room_name_(room_name) {
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    display_field_ = new wxRichTextCtrl(this, wxID_ANY, "",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    display_field_->SetBackgroundColour(wxColour(240, 240, 240));

    // Минимальная высота поля сообщений
    display_field_->SetMinSize(wxSize( 100, 300));

    //Установка шрифта по умолчанию
    SetFont(DEFAULT_FONT);
    

    sizer->Add(display_field_, 1, wxEXPAND);
    SetSizer(sizer);
}

void ChatRoomPanel::AddMessage(const IncomingMessage& msg) {
    wxRichTextAttr attr;
    attr.SetFontSize(10);

    //Отдельный формат для системных сообщений
    if (msg.sender == SYSTEM_SENDER_NAME) {
        attr.SetTextColour(*wxBLUE);
        attr.SetFontWeight(wxFONTWEIGHT_BOLD);
    } else {
        attr.SetTextColour(*wxBLACK);
    }
    
    // Время сообщения
    display_field_->BeginStyle(attr);
    display_field_->WriteText(msg.formatted_time() + " - ");
    display_field_->EndStyle();

    // Отправитель
    wxRichTextAttr sender_attr = attr;
    sender_attr.SetTextColour(wxColour(0, 128, 0)); // темно-зеленый

    display_field_->BeginStyle(sender_attr);
    display_field_->WriteText(msg.sender + " - ");
    display_field_->EndStyle();

    // Текст сообщения с BBCode
    wxRichTextAttr text_attr;
    text_attr.SetFontSize(10);
    text_attr.SetTextColour(*wxBLACK);
    display_field_->BeginStyle(text_attr);

    ParseBBCode(wxString(msg.text));  // Парсинг BBCode

    display_field_->EndStyle();

    // Разделитель
    wxRichTextAttr separator_attr;
    separator_attr.SetTextColour(wxColour(200, 200, 200));
    display_field_->BeginStyle(separator_attr);
    display_field_->WriteText("\n" + wxString('-', 80) + "\n");
    display_field_->EndStyle();

    // Прокрутка вниз
    display_field_->ShowPosition(display_field_->GetLastPosition());
}

void ChatRoomPanel::Clear() {
    display_field_->Clear();
}

void ChatRoomPanel::ParseBBCode(const wxString& text) {
    bbcode::ParseBBCode(text, display_field_);
}

MainWindow::MainWindow(std::unique_ptr<client::ChatClient> client,
    const std::string username, const std::string& hash_password) :
    wxFrame(nullptr, wxID_ANY, "Чат клиента",wxDefaultPosition, wxSize(800,600)), 
    client_(std::move(client)),
    default_font_(DEFAULT_SERVER),
    current_username_(username),
    hash_password_(hash_password)
    {
    ConstructInterface();
    client_->LoginUser(current_username_, hash_password_);
    client_->RequestUsersForRoom(MAIN_ROOM_NAME);
}

void MainWindow::ConstructInterface() {

    //создаем главную панель
    wxPanel* main_panel = new wxPanel(this);

    //создаем главный вертикальный контейнер
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    //устанавливаем шрифт по умолчанию
    this->SetFont(default_font_);

    //Верхняя часть - вкладки комнат 
    room_notebook_ = new wxNotebook(main_panel, wxID_ANY);
    // Минимальная высота 300px
    room_notebook_->SetMinSize(wxSize(-1, 300));

    //список пользователей
    wxBoxSizer* content_sizer = new wxBoxSizer(wxHORIZONTAL);
    content_sizer->Add(room_notebook_, 1, wxEXPAND | wxRIGHT, 5);

    // Вертикальный контейнер для списка пользователей
    wxBoxSizer* users_sizer = new wxBoxSizer(wxVERTICAL);

    // Заголовок с именем комнаты
    room_users_label_ = new wxStaticText(main_panel, wxID_ANY,
        "Пользователи комнаты:");
    room_users_label_->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT,
        wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    users_sizer->Add(room_users_label_, 0, wxEXPAND | wxBOTTOM, 5);

    // Список пользователей
    users_listbox_ = new wxListBox(main_panel, wxID_ANY,
        wxDefaultPosition, wxSize(200, -1));
    users_sizer->Add(users_listbox_, 1, wxEXPAND);

    content_sizer->Add(users_sizer, 0, wxEXPAND);
    main_sizer->Add(content_sizer, 1, wxEXPAND | wxALL, 5);

    //Средняя часть - история сообщений
    ChatRoomPanel* room_panel = new ChatRoomPanel(room_notebook_, MAIN_ROOM_NAME);
    room_notebook_->AddPage(room_panel, MAIN_ROOM_NAME, true);
    rooms_[MAIN_ROOM_NAME] = room_panel;


    //Нижняя часть - элементы управления
    wxPanel* bottom_panel = new wxPanel(main_panel);
    wxBoxSizer* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);

    //Левая часть - стили текста и поле ввода
    wxPanel* left_panel = new wxPanel(bottom_panel);
    wxBoxSizer* left_sizer = new wxBoxSizer(wxVERTICAL);

    //Панель стилей текста
    wxPanel* style_panel = new wxPanel(left_panel);
    wxBoxSizer* style_sizer = new wxBoxSizer(wxHORIZONTAL);

    text_style_bold_button_ = new wxButton(style_panel, wxID_ANY, "B", wxDefaultPosition, wxSize(30, 30));
    text_style_italic_button_ = new wxButton(style_panel, wxID_ANY, "I", wxDefaultPosition, wxSize(30, 30));
    text_style_underline_button_ = new wxButton(style_panel, wxID_ANY, "U", wxDefaultPosition, wxSize(30, 30));
    text_style_smiley_button_ = new wxButton(style_panel, wxID_ANY, "☺", wxDefaultPosition, wxSize(30, 30));

    style_sizer->Add(text_style_bold_button_, 0, wxRIGHT, 5);
    style_sizer->Add(text_style_italic_button_, 0, wxRIGHT, 5);
    style_sizer->Add(text_style_underline_button_, 0, wxRIGHT, 5);
    style_sizer->Add(text_style_smiley_button_, 0);

    style_sizer->AddStretchSpacer();//выравнивание по правому краю


    //панель для счетчика с иконкой информации
    wxPanel* counter_panel = new wxPanel(style_panel);
    wxBoxSizer* counter_sizer = new wxBoxSizer(wxHORIZONTAL);

    //добавляем иконку информации
    wxStaticBitmap* info_icon = new wxStaticBitmap(
        counter_panel, wxID_ANY,
        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_MENU),
        wxDefaultPosition, wxSize(16, 16)
    );

    // Элемент для отображения длины сообщения
    message_length_label_ = new wxStaticText(counter_panel, wxID_ANY,
        wxString::Format("0 / %d", MAX_MESSAGE_LENGTH));

    // Фиксируем ширину на основе максимально возможной длины
    wxString max_length_text = wxString::Format("9999 / %d", MAX_MESSAGE_LENGTH);
    wxSize text_size = message_length_label_->GetTextExtent(max_length_text);
    message_length_label_->SetMinSize(text_size);
    message_length_label_->SetLabel("0 / 0"); // Сбрасываем начальное значение

    counter_sizer->Add(info_icon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    counter_sizer->Add(message_length_label_, 0, wxALIGN_CENTER_VERTICAL);
    counter_panel->SetSizer(counter_sizer);

    style_sizer->Add(counter_panel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);

    style_panel->SetSizer(style_sizer);
    left_sizer->Add(style_panel, 0, wxEXPAND | wxBOTTOM, 10);

    // Добавляем всплывающую подсказку
    wxString tooltip = wxString::Format(
        "Ограничение длины сообщения: %d символов\n"
        "Учитываются все символы кроме переносов строк",
        MAX_MESSAGE_LENGTH
    );
    counter_panel->SetToolTip(tooltip);
    info_icon->SetToolTip(tooltip);
    message_length_label_->SetToolTip(tooltip);

    //поле ввода сообщений и кнопка отправки
    wxPanel* input_panel = new wxPanel(left_panel);
    wxBoxSizer* input_sizer = new wxBoxSizer(wxHORIZONTAL);

    input_field_ = new wxRichTextCtrl(input_panel, wxID_ANY, "",
        wxDefaultPosition, wxSize(-1, 100),
        wxTE_MULTILINE);
    input_field_->SetBackgroundColour(wxColour(255, 255, 230));

    //настройка базовых стилей
    wxRichTextAttr attr;
    attr.SetFont(default_font_);
    input_field_->SetBasicStyle(attr);
    

    //кнопка отправки
    send_message_button_ = new wxButton(input_panel, wxID_ANY, "Отправить");
    send_message_button_->SetMinSize(wxSize(100, -1));

    input_sizer->Add(input_field_, 1, wxEXPAND);
    input_sizer->Add(send_message_button_, 0, wxLEFT | wxALIGN_BOTTOM, 10);

    input_panel->SetSizer(input_sizer);
    left_sizer->Add(input_panel, 1, wxEXPAND);

    left_panel->SetSizer(left_sizer);
    bottom_sizer->Add(left_panel,1, wxEXPAND | wxRIGHT, 10);

    //правая часть - кнопки управления
    wxPanel* right_panel = new wxPanel(bottom_panel);
    wxBoxSizer* right_sizer = new wxBoxSizer(wxVERTICAL);

    create_room_button_ = new wxButton(right_panel, wxID_ANY, "Создать комнату");
    room_list_button_ = new wxButton(right_panel, wxID_ANY, "Список комнат");
    leave_room_button_ = new wxButton(right_panel, wxID_ANY, "Выйти из комнаты");
    change_username_button_ = new wxButton(right_panel, wxID_ANY, "Изменить имя");
    logout_button_ = new wxButton(right_panel, wxID_ANY, "Выйти из приложения");

    //Ставим минимальной ширины для кнопок
    int min_width = 200;
    create_room_button_->SetMinSize(wxSize(min_width, -1));
    room_list_button_->SetMinSize(wxSize(min_width, -1));
    leave_room_button_->SetMinSize(wxSize(min_width, -1));
    change_username_button_->SetMinSize(wxSize(min_width, -1));
    logout_button_->SetMinSize(wxSize(min_width, -1));

    right_sizer->Add(create_room_button_, 0, wxEXPAND | wxBOTTOM, 5);
    right_sizer->Add(room_list_button_, 0, wxEXPAND | wxBOTTOM, 5);
    right_sizer->Add(leave_room_button_, 0, wxEXPAND | wxBOTTOM, 5);
    right_sizer->Add(change_username_button_, 0, wxEXPAND | wxBOTTOM, 5);
    right_sizer->Add(logout_button_, 0, wxEXPAND);

    right_panel->SetSizer(right_sizer);
    bottom_sizer->Add(right_panel, 0, wxEXPAND);

    bottom_panel->SetSizer(bottom_sizer);
    main_sizer->Add(bottom_panel, 0, wxEXPAND | wxALL, 5);

    main_panel->SetSizer(main_sizer);

    //Настройка обработчика сообщений
    client_->SetLoginHandler([this](const std::string& name) {
        CallAfter([this, name]() { SetTitleMainWindow(name); });
        });
    client_->SetMessageHandler([this](const IncomingMessage& msg) {
        CallAfter([this, msg]() { AddMessage(msg); });
        });
    client_->SetRoomListHandler([this](const std::set<std::string>& rooms) {
        CallAfter([this, rooms]() { UpdateRoomList(rooms); });
        });
    client_->SetRoomCreateHandler([this](bool success, const std::string& room_name) {
        CallAfter([this, success, room_name]() { CreateRoom(success, room_name); });
        });
    client_->SetRoomEnterHandler([this](bool success, const std::string& room_name) {
        CallAfter([this, success, room_name]() { EnterRoom(success, room_name); });
        });
    client_->SetRoomExitHandler([this](bool success, const std::string& room_name) {
        CallAfter([this, success, room_name]() { LeaveRoom(success, room_name); });
        });
    client_->SetChangeNameHandler([this](bool success, const std::string& name) {
        CallAfter([this, success, name]() { ChangeName(success, name); });
        });
    client_->SetRoomUsersHandler([this](const std::string& room,
        const std::set<std::string>& users) {
            CallAfter([this, room, users]() { UpdateRoomUsers(room, users); });
        });
    client_->SetOtherUserNewNameHandler([this](const std::string& old_name, const std::string& new_name) {
        CallAfter([this, old_name, new_name]() { UpdateInterfaceAfterChangedName(old_name, new_name); });
        });

    //Биндинг событий
    Bind(wxEVT_CLOSE_WINDOW, &MainWindow::OnClose, this);
    room_notebook_->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &MainWindow::OnTabChanged, this);
    send_message_button_->Bind(wxEVT_BUTTON, &MainWindow::OnSendMessage, this);
    create_room_button_->Bind(wxEVT_BUTTON, &MainWindow::OnCreateRoom, this);
    room_list_button_->Bind(wxEVT_BUTTON, &MainWindow::OnRoomList, this);
    leave_room_button_->Bind(wxEVT_BUTTON, &MainWindow::OnLeaveRoom, this);
    change_username_button_->Bind(wxEVT_BUTTON, &MainWindow::OnChangedUserName, this);
    logout_button_->Bind(wxEVT_BUTTON, &MainWindow::OnLogout, this);
    text_style_bold_button_->Bind(wxEVT_BUTTON, &MainWindow::OnTextFormatBold, this);
    text_style_italic_button_->Bind(wxEVT_BUTTON, &MainWindow::OnTextFormatItalic, this);
    text_style_underline_button_->Bind(wxEVT_BUTTON, &MainWindow::OnTextFormatUnderline, this);
    text_style_smiley_button_->Bind(wxEVT_BUTTON, &MainWindow::OnSmiley, this);
    input_field_->Bind(wxEVT_TEXT, &MainWindow::OnTextChanged, this);//для подсчета длинны сообщений
    users_listbox_->Bind(wxEVT_CONTEXT_MENU, &MainWindow::OnUserListRightClick, this);

    Center();
    Maximize();
}

void MainWindow::SetTitleMainWindow(const std::string& name) {
    // Защита на случай пустого имени
    if (current_username_.empty()) {
        current_username_ = "Неизвестный пользователь";
    }
    else {
        current_username_ = name;
    }

    // Устанавливаем заголовок
    SetTitle(wxString::Format("Чат клиента - %s", current_username_));
}

void MainWindow::OnSendMessage(wxCommandEvent& event) {
    wxString text = input_field_->GetValue();
    if (!text.IsEmpty()) {

        //проверяем длину сообщения
        int  useful_count = CountUsefulChars(text);
        if (useful_count > MAX_MESSAGE_LENGTH) {
            wxMessageBox(
                wxString::Format("Сообщение слишком длинное! Максимум %d символов.\n\nСимволов: %d",
                    MAX_MESSAGE_LENGTH, useful_count), "Ошибка", wxICON_WARNING);
            return;
        }

        int selection = room_notebook_->GetSelection();
        if (selection != wxNOT_FOUND) {
            wxString room_name = room_notebook_->GetPageText(selection);

            //преобразуем в bbcode
            wxString bbcode_text = ConvertRichTextToBBCode(input_field_);

            OutgoingMessage msg;
            msg.room = room_name.ToStdString();
            msg.text = bbcode_text.ToStdString();

            client_->SendMessageToServer(msg);
            input_field_->Clear();

            //reverse msg for private 
            IncomingMessage reverse_msg;
            reverse_msg.room = msg.room;
            reverse_msg.text = std::move(msg.text);
            reverse_msg.sender = msg.room;
            reverse_msg.timestamp = std::chrono::system_clock::now();
            AddMessage(reverse_msg);

            //После отправки сбрасываем стили
            wxRichTextAttr reset_attr;
            reset_attr.SetFont(default_font_);
            input_field_->SetBasicStyle(reset_attr);
            input_field_->SetDefaultStyle(reset_attr);
        }
    } 
    //Вернуть фокус в поле ввода
    input_field_->SetFocus();  
}

void MainWindow::OnCreateRoom(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Введите название комнаты:", "Создание комнаты");
    if (dlg.ShowModal() == wxID_OK) {
        wxString room_name = dlg.GetValue();
        if (!room_name.IsEmpty()) {
            //Эмуляция запроса на сервер
            client_->CreateRoom(room_name.ToStdString());
        }
    }
}

void MainWindow::OnRoomList(wxCommandEvent& event) {
    client_->RequestRoomList();
}

void MainWindow::OnUserList(wxCommandEvent& event) {
    
}

void MainWindow::OnLeaveRoom(wxCommandEvent& event) {
    int selection = room_notebook_->GetSelection();
    if (selection == wxNOT_FOUND) {
        wxMessageBox("Выберите комнату для выхода", "Ошибка", wxICON_WARNING);
        return;
    }

    wxString room_name = room_notebook_->GetPageText(selection);

    if (room_name == MAIN_ROOM_NAME) {
        wxMessageBox("Нельзя выйти из основной комнаты", "Ошибка", wxICON_WARNING);
        return;
    }
    else {
        client_->LeaveRoom(room_name.ToStdString());
    }  
}

void MainWindow::OnChangedUserName(wxCommandEvent& event) {
    wxTextEntryDialog dlg(this, "Введите новое имя пользователя:", "Смена имени", current_username_);

    if (dlg.ShowModal() == wxID_OK) {
        wxString new_name = dlg.GetValue();
        if (!new_name.IsEmpty()) {
            if (new_name.at(0) != '@' && "@" + new_name != current_username_) {
                std::string new_current_name = '@' + new_name.ToStdString();
                client_->ChangeUsername(new_current_name);
            }
            else {
                wxMessageBox("Имя не должно начинаться с \'@\'", "Ошибка", wxICON_WARNING);
            }
            
        }
    }
}

void MainWindow::CreateRoom(bool success, const std::string& room_name) {
    if (success) {
        if (rooms_.find(room_name) == rooms_.end()) {
            client_->JoinRoom(room_name);
        }
        else {
            wxMessageBox("Комната с таким именем уже существует", "Ошибка", wxICON_WARNING);
        }
    }
    else {
        wxMessageBox("Ошибка при создании комнаты", "Ошибка", wxICON_WARNING);
    }
    
}

void MainWindow::AddMessage(const IncomingMessage& msg) {
    //для приватных сообщений
    if (msg.room == current_username_) {
        std::string new_room_name = msg.sender;

        //Если комнаты не существует, то созданим ее
        if (rooms_.find(new_room_name) == rooms_.end()) {
            std::cout << "Create room " << new_room_name << "\n";
            EnterRoom(true, new_room_name);
        }

        IncomingMessage modify_msg;
        modify_msg.room = new_room_name;
        modify_msg.text = msg.text;
        modify_msg.sender = msg.sender;
        modify_msg.timestamp = msg.timestamp;

        //Отправляем сообщение в комнату
        rooms_[new_room_name]->AddMessage(modify_msg);
    }
    else {
        //Если комнаты не существует, то созданим ее
        if (rooms_.find(msg.room) == rooms_.end()) {
            std::cout << "Create room " << msg.room << "\n";
            EnterRoom(true, msg.room);
        }

        //Отправляем сообщение в комнату
        rooms_[msg.room]->AddMessage(msg);
    }   
}

void MainWindow::OnTabChanged(wxNotebookEvent& event) {
    int selection = room_notebook_->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxString room_name = room_notebook_->GetPageText(selection);
        //UpdateUserListForRoom(room_name.ToStdString());
        client_->RequestUsersForRoom(room_name.ToStdString());
    }
    event.Skip();
}

void MainWindow::UpdateRoomUsers(const std::string& room_name, const std::set<std::string>& users) {
    if (room_notebook_->GetPageText(room_notebook_->GetSelection()).ToStdString() == room_name) {
        users_listbox_->Clear();
        for (const auto& user : users) {
            wxString label = user;
            if (user == current_username_) {
                label += " (Вы)";
            }
            users_listbox_->Append(label);
        }
    }
}

void MainWindow::UpdateInterfaceAfterChangedName(const std::string& old_name, const std::string& new_name) {
    std::cout << old_name << " сменил имя на " << new_name << '\n';
    // 1. Отправляем уведомление в основную комнату
    IncomingMessage msg;
    msg.room = MAIN_ROOM_NAME;
    msg.sender = SYSTEM_SENDER_NAME;
    msg.text = old_name + " сменил имя на " + new_name;
    msg.timestamp = std::chrono::system_clock::now();
    AddMessage(msg);

    // 2. Обработка приватных комнат
    // Создаем копию карты комнат для безопасной итерации
    auto rooms_copy = rooms_;

    // Обновляем вкладку
    for (size_t i = 0; i < room_notebook_->GetPageCount(); ++i) {
        wxWindow* page = room_notebook_->GetPage(i);
        if (auto room_panel = dynamic_cast<ChatRoomPanel*>(page)) {
            if (room_panel->GetRoomName() == old_name) {
                // Обновляем текст вкладки
                room_notebook_->SetPageText(i, new_name);

                // Обновляем имя комнаты в панели
                room_panel->SetRoomName(new_name);

                // Обновляем карту комнат
                rooms_.erase(old_name);
                rooms_[new_name] = room_panel;

                break;
            }
        }
    }

    // 3. Обновляем текущее имя пользователя в интерфейсе
    SetTitle(wxString::Format("Чат клиента - %s", current_username_));

    // 4. Обновляем список пользователей в текущей комнате
    int selection = room_notebook_->GetSelection();
    if (selection != wxNOT_FOUND) {
        if (auto page = dynamic_cast<ChatRoomPanel*>(room_notebook_->GetPage(selection))) {
            client_->RequestUsersForRoom(page->GetRoomName());
        }
    }
}

void MainWindow::OnLogout(wxCommandEvent& event) {
    if (wxMessageBox("Вы уверены, что хотите выйти?", "Подтверждение",
        wxYES_NO | wxICON_QUESTION) == wxYES) {
        Close(true);
    }
}

void MainWindow::OnClose(wxCloseEvent& event) {
    Destroy();
    wxExit();
}

void MainWindow::OnTextFormatBold(wxCommandEvent& event) {
    wxRichTextAttr attr;
    attr.SetFontWeight(wxFONTWEIGHT_BOLD);
    ApplyTextStyle(attr);
}

void MainWindow::OnTextFormatItalic(wxCommandEvent& event) {
    wxRichTextAttr attr;
    attr.SetFontStyle(wxFONTSTYLE_ITALIC);
    ApplyTextStyle(attr);
}

void MainWindow::OnTextFormatUnderline(wxCommandEvent& event) {
    wxRichTextAttr attr;
    attr.SetFontUnderlined(true);
    ApplyTextStyle(attr);
}

void MainWindow::OnSmiley(wxCommandEvent& event) {
    //список смайликов
    auto smileys = bbcode::GetSmileys();

    wxMenu menu;
    std::map<int, wxString> id_to_smiley;
    for (const auto& smiley : smileys) {
        int id = wxWindow::NewControlId();
        menu.Append(id, smiley.emoji);
        id_to_smiley[id] = smiley.emoji;
    }

        // Обработчик с установкой фокуса
    menu.Bind(wxEVT_MENU, [this, id_to_smiley](wxCommandEvent& e) {
        auto it = id_to_smiley.find(e.GetId());
        if (it == id_to_smiley.end()) return;

        wxString smiley = it->second;

        // Сохраняем текущую позицию курсора
        long start, end;
        input_field_->GetSelection(&start, &end);

        // Устанавливаем фокус на поле ввода
        input_field_->SetFocus();

        // Вставляем смайлик
        input_field_->WriteText(smiley);

        // Восстанавливаем позицию курсора
        if (start == end) {
            input_field_->SetInsertionPoint(start + smiley.length());
        }
        else {
            input_field_->SetSelection(start + smiley.length(), start + smiley.length());
        }
   
        });

    text_style_smiley_button_->PopupMenu(&menu);
}

void MainWindow::ApplyTextStyle(const wxTextAttr& attr) {
    if (input_field_->HasSelection()) {
        long start, end;
        input_field_->GetSelection(&start, & end);
        input_field_->SetStyle(start, end, attr);
    } else {
        wxTextAttr current_attr = input_field_->GetDefaultStyle();
        wxTextAttr new_attr = current_attr;
        new_attr.Merge(attr);
        input_field_->SetDefaultStyle(new_attr);
    }
}

void MainWindow::OnTextChanged(wxCommandEvent& event) {
    wxString text = input_field_->GetValue();
    int useful_count = CountUsefulChars(text);

    wxString label = wxString::Format("%d / %d", useful_count, MAX_MESSAGE_LENGTH);
    message_length_label_->SetLabel(label);

    if (useful_count > MAX_MESSAGE_LENGTH) {
        message_length_label_->SetForegroundColour(*wxRED);
    }
    else {
        message_length_label_->SetForegroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    }

    event.Skip();
}

wxString MainWindow::ConvertRichTextToBBCode(wxRichTextCtrl* ctrl) {
    return bbcode::ConvertRichTextToBBCode(ctrl);
}

int MainWindow::CountUsefulChars(const wxString& text) const {
    int count = 0;
    constexpr int max_safe = std::numeric_limits<int>::max();

    for (wxUniChar c : text) {
        // Игнорируем только управляющие символы и символы форматирования
        if (c == '\n' || c == '\r' || c == '\t') continue;

        // Учитываем ВСЕ остальные символы (включая смайлы, пунктуацию и пробелы)
        count++;

        // Защита от переполнения
        if (count >= max_safe) {
            return max_safe;
        }
    }
    return count;
}

bool MainWindow::IsPrivateRoom(const std::string& name) const {
    return name.size() > 2 && name[0] == '@' && name[1] == '@';
}

void MainWindow::OnUserListRightClick(wxContextMenuEvent& event) {
    int selection = users_listbox_->GetSelection();
    if (selection == wxNOT_FOUND) return;

    wxString selected_user = users_listbox_->GetString(selection);

    // Проверка, что это не текущий пользователь
    if (selected_user.EndsWith("(Вы)")) {
        wxMessageBox("Нельзя создать приватный чат с самим собой", "Ошибка", wxICON_WARNING);
        return;
    }

    // Удаляем пометку "(Вы)" если есть
    selected_user.Replace(" (Вы)", "", true);

    // Создаем контекстное меню
    wxMenu menu;
    menu.Append(CustomIDs::ID_CREATE_PRIVATE_CHAT, "Создать чат с " + selected_user);

    // Обработчик выбора пункта меню
    menu.Bind(wxEVT_MENU, [this, selected_user](wxCommandEvent&) {
        CreatePrivateChat(selected_user);
        }, CustomIDs::ID_CREATE_PRIVATE_CHAT);

    // Показываем меню в позиции курсора
    users_listbox_->PopupMenu(&menu);
}



void MainWindow::UpdateRoomList(const std::set<std::string>& rooms) {
    // Получаем список уже открытых комнат
    std::set<wxString> existing_rooms;
    for (size_t i = 0; i < room_notebook_->GetPageCount(); i++) {
        existing_rooms.insert(room_notebook_->GetPageText(i));
    }

    std::set<std::string> realrooms;
    for (const auto& rr : rooms) {
        if (rr.size() > 1 && rr[0] != '@' && rr[1] != '@') {
            realrooms.insert(rr);
        }
    }

    // Создаем диалог выбора комнат
    ListSelectionDialog dlg(this, "Доступные комнаты", realrooms, existing_rooms);

    if (dlg.ShowModal() == wxID_OK) {
        wxString room_name = dlg.GetSelectedItem();
        if (!room_name.empty()) {
            // Отправляем запрос на присоединение к комнате
            client_->JoinRoom(room_name.ToStdString());
        }
    }
}

void MainWindow::EnterRoom(bool success, const std::string& room_name) {
    if (success) {
        std::cerr <<"[MAIN]: " << current_username_ << " вошел в комнату \"" << room_name << "\"\n";
        ChatRoomPanel* room_panel = new ChatRoomPanel(room_notebook_, room_name);
        room_notebook_->AddPage(room_panel, room_name, true);
        rooms_[room_name] = room_panel;
        client_->RequestUsersForRoom(room_name);
    }
}

void MainWindow::LeaveRoom(bool success, const std::string& room_name) {
    std::cout << " leave room \n";
    if (success && room_name != MAIN_ROOM_NAME) {
        std::cerr << "[MAIN]: " << current_username_ << " вышел из комнаты \"" << room_name << "\"\n";

        for (size_t i = 0; i < room_notebook_->GetPageCount(); ++i) {
            wxWindow* page = room_notebook_->GetPage(i);
            if (auto room_panel = dynamic_cast<ChatRoomPanel*>(page)) {
                if (room_panel->GetRoomName() == room_name) {
                    room_notebook_->DeletePage(i);
                    rooms_.erase(room_name);
                    return;
                }
            }
        }
    }   
}

void MainWindow::ChangeName(bool success, const std::string& new_name) {
    std::cout << " change name \n";
    if (success) {
        current_username_ = new_name;
        SetTitle(wxString::Format("Чат клиента - %s", current_username_));

    }
}

void MainWindow::CreatePrivateChat(const wxString& username) {
    EnterRoom(true, username.ToStdString());
}

}// end namespace gui