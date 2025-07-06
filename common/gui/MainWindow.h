#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/font.h>
#include <wx/encinfo.h>
#include <wx/taskbar.h>
#include <wx/artprov.h>
#include <map>
#include <algorithm>
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
    const std::string& GetRoomName() const { return room_name_; }
    void SetRoomName(const std::string& name) { room_name_ = name; }

private:
    wxRichTextCtrl* display_field_;
    std::string room_name_;

    void ParseBBCode(const wxString& text);
};

class StyleHandler {
public:
    enum State {
        INACTIVE = 0,
        ACTIVE = 1,
        MIXED = 2
    };

    virtual ~StyleHandler() = default;

    virtual void Apply(wxRichTextAttr& attr, bool activate) const = 0;
    virtual bool IsActive(const wxRichTextAttr& attr) const = 0;
    virtual long GetStyleFlags() const = 0;
    virtual wxString GetName() const = 0;
    virtual State GetState(const wxRichTextAttr& attr) const {
        return IsActive(attr) ? ACTIVE : INACTIVE;
    }
};

// Конкретные реализации для каждого стиля
class BoldHandler : public StyleHandler {
public:
    void Apply(wxRichTextAttr& attr, bool activate) const override {
        attr.SetFontWeight(activate ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    }

    bool IsActive(const wxRichTextAttr& attr) const override {
        return attr.GetFontWeight() == wxFONTWEIGHT_BOLD;
    }

    long GetStyleFlags() const override {
        return wxTEXT_ATTR_FONT_WEIGHT;
    }

    wxString GetName() const override { return "Bold"; }

    State GetState(const wxRichTextAttr& attr) const override {
        if (!(attr.HasFontWeight())) return MIXED;
        return attr.GetFontWeight() == wxFONTWEIGHT_BOLD ? ACTIVE : INACTIVE;
    }
};

class ItalicHandler : public StyleHandler {
public:
    void Apply(wxRichTextAttr& attr, bool activate) const override {
        attr.SetFontStyle(activate ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL);
    }

    bool IsActive(const wxRichTextAttr& attr) const override {
        return attr.GetFontStyle() == wxFONTSTYLE_ITALIC;
    }

    long GetStyleFlags() const override {
        return wxTEXT_ATTR_FONT_ITALIC;
    }

    wxString GetName() const override { return "Italic"; }

    State GetState(const wxRichTextAttr& attr) const override {
        // Проверяем, установлен ли атрибут стиля
        if (!(attr.GetFlags() & wxTEXT_ATTR_FONT_ITALIC)) return MIXED;
        return attr.GetFontStyle() == wxFONTSTYLE_ITALIC ? ACTIVE : INACTIVE;
    }
};

class UnderlineHandler : public StyleHandler {
public:
    void Apply(wxRichTextAttr& attr, bool activate) const override {
        attr.SetFontUnderlined(activate);
    }

    bool IsActive(const wxRichTextAttr& attr) const override {
        return attr.GetFontUnderlined();
    }

    long GetStyleFlags() const override {
        return wxTEXT_ATTR_FONT_UNDERLINE;
    }

    wxString GetName() const override { return "Underline"; }

    State GetState(const wxRichTextAttr& attr) const override {
        if (!(attr.GetFlags() & wxTEXT_ATTR_FONT_UNDERLINE)) return MIXED;
        return attr.GetFontUnderlined() ? ACTIVE : INACTIVE;
    }
};

class MainWindow : public wxFrame {
public:
    MainWindow(std::unique_ptr<client::ChatClient> client, 
        const std::string username, const std::string& hash_password);

private:
    std::unique_ptr<client::ChatClient> client_;
    wxRichTextCtrl* input_field_;
    wxNotebook* room_notebook_;
    std::map<std::string, ChatRoomPanel*> rooms_;
    wxFont default_font_;
    wxStaticText* message_length_label_;
    std::string current_username_;
    std::string hash_password_;

    //tray
    wxTaskBarIcon* tray_icon_ = nullptr;

    //списки пользователей
    wxListBox* users_listbox_;
    wxStaticText* room_users_label_;

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
    std::unique_ptr<StyleHandler> bold_handler;
    std::unique_ptr<StyleHandler> italic_handler;
    std::unique_ptr<StyleHandler> underline_handler;

    wxRichTextAttr current_default_style_;


    void ConstructInterface();
    void SetTitleMainWindow(const std::string& name);
    void OnSendMessage(wxCommandEvent& event);
    void OnCreateRoom(wxCommandEvent& event);
    void OnRoomList(wxCommandEvent& event);
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

    void UpdateRoomUsers(const std::string& room_name, const std::set<std::string>& users);
    void UpdateInterfaceAfterChangedName(const std::string& old_name, const std::string& new_name);

    void InsertTextAtCaret(const wxString& text);
    void OnTextChanged(wxCommandEvent& event);
    void OnTextSelectionChanged(wxEvent& event);
    void ToggleStyle(StyleHandler& handler);
    void UpdateButtonStates();
    void UpdateButtonAppearance(wxButton* button, StyleHandler::State state, const wxString& name);
    void ResetTextStyles(bool updateUI = true);

    bool IsNonOnlySpace(const wxString& text);

    void OnUserListRightClick(wxContextMenuEvent& event);
    void CreatePrivateChat(const wxString& username);

    void UpdateRoomList(const std::set<std::string>& rooms);
    void CreateRoom(bool success, const std::string& room_name);
    void EnterRoom(bool success, const std::string& room_name);
    void LeaveRoom(bool success, const std::string& room_name);
    void ChangeName(bool success, const std::string& room_name);

    wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl);
    int CountUsefulChars(const wxString& text) const;
    void OnKeyDown(wxKeyEvent& event);

    //методы для взаимодействия с треем
    void CreateTrayIcon();
    void RemoveTrayIcon();
    void RestoreFromTray();
    void OnTrayIconDoubleClick(wxTaskBarIconEvent& event);

    //для удаления смайлика
    bool IsPartOfEmoji(long pos) const;
    long FindEmojiStart(long pos) const;
    long FindEmojiEnd(long pos) const;
    void DeleteEmojiAtPosition(long pos);
};

}//end namespace gui