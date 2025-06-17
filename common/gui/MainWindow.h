#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/font.h>
#include <wx/encinfo.h>
#include <map>
#include "LoginDialog.h"
#include "ChatClient.h"
#include "BBCodeUtils.h"
#include "ListSelectionDialog.h"

namespace gui{

class ChatRoomPanel : public wxPanel {
public:
    ChatRoomPanel(wxWindow* parent, const std::string& room_name);

    void AddMessage(const IncomingMessage& msg);
    void Clear();

private:
    wxRichTextCtrl* display_field_;

    void ParseBBCode(const wxString& text);
};

class MainWindow : public wxFrame {
public:
    MainWindow(std::unique_ptr<client::ChatClient> client);

private:
    std::unique_ptr<client::ChatClient> client_;
    wxRichTextCtrl* input_field_;
    wxNotebook* room_notebook_;
    std::map<std::string, ChatRoomPanel*> rooms_;
    wxFont default_font_;
    wxStaticText* message_length_label_;
    std::string current_username_; //колхоз пока нет сервера тут будет локальное имя пользователя

    //UI кнопки управления
    wxButton* send_message_button_;
    wxButton* create_room_button_;
    wxButton* room_list_button_;
    wxButton* user_list_button_;
    wxButton* leave_room_button_;
    wxButton* change_username_button_;
    wxButton* logout_button_;

    //UI элементы стили текста
    wxButton* text_style_bold_button_;
    wxButton* text_style_italic_button_;
    wxButton* text_style_underline_button_;
    wxButton* text_style_smiley_button_;

    void ConstructInterface();
    void OnSendMessage(wxCommandEvent& event);
    void OnCreateRoom(wxCommandEvent& event);
    void OnRoomList(wxCommandEvent& event);
    void OnUserList(wxCommandEvent& event);
    void OnLeaveRoom(wxCommandEvent& event);
    void OnChangedUserName(wxCommandEvent& event);
    void OnLogout(wxCommandEvent& event);
    void OnTextFormatBold(wxCommandEvent& event);
    void OnTextFormatItalic(wxCommandEvent& event);
    void OnTextFormatUnderline(wxCommandEvent& event);
    void OnSmiley(wxCommandEvent& event);
    void OnTabChanged(wxNotebookEvent& event);
    void OnClose(wxCloseEvent& event);
    void AddMessage(const IncomingMessage& msg);

    void ApplyTextStyle(const wxTextAttr& attr);
    void OnTextChanged(wxCommandEvent& event);

    void UpdateRoomList(const std::vector<std::string>& rooms);
    void CreateRoom(bool success, const std::string& room_name);
    void EnterRoom(bool success, const std::string& room_name);
    void LeaveRoom(bool success, const std::string& room_name);

    wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl);
    int CountUsefulChars(const wxString& text) const;
};

}//end namespace gui