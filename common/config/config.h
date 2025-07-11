#pragma once
#include <string>
#include <set>
#include <wx/wx.h>
#include <wx/settings.h>
#include <wx/font.h>
#include <wx/fontenum.h>
#include <vector>


const std::string MAIN_ROOM_NAME = "general";
const std::string SYSTEM_SENDER_NAME = "System";
const wxFont DEFAULT_FONT_SECOND(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Segoe UI Emoji");
const wxFont DEFAULT_FONT(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial Unicode MS");
const std::string DEFAULT_SERVER = "127.0.0.1";
const std::string CLIENT_FILE_CONFIG = "/irc_chat/client_config/chat_servers.ini"; //as default to user_home_dir + \AppData\Roaming
const int MAX_MESSAGE_LENGTH = 512;
const std::string DEF_SERVER = "127.0.0.1";
const std::string CLIENT_FIRST_PORT = "9003";
const std::string CLIENT_SECOND_PORT = "9002";

static const std::set<wxUniChar> AllowedChars = [] {
    std::set<wxUniChar> chars;

    // Цифры
    for (wxChar c = '0'; c <= '9'; ++c) chars.insert(c);

    // Английские буквы верхнего регистра
    for (wxChar c = 'A'; c <= 'Z'; ++c) chars.insert(c);

    // Английские буквы нижнего регистра
    for (wxChar c = 'a'; c <= 'z'; ++c) chars.insert(c);

    // Специальные символы
    const wxString specials = "!#%?*()_-+=<>";
    for (wxUniChar c : specials) chars.insert(c);

    return chars;
    }();

class FontManager {
public:
    // список лицевых имён для обычного GUI-текста
    static const std::vector<std::string>& GuiFontCandidates() {
        static std::vector<std::string> v = {
#ifdef _WIN32
            "Segoe UI", "Tahoma", "Verdana", "Arial"
#elif __APPLE__
            "Helvetica Neue", "Arial"
#else // Linux
            "Noto Sans", "DejaVu Sans", "FreeSans"
#endif
        };
        return v;
    }
    // список лицевых имён для Emoji
    static const std::vector<std::string>& EmojiFontCandidates() {
        static std::vector<std::string> v = {
#ifdef _WIN32
            "Arial Unicode MS","Segoe UI Emoji", "Segoe UI Symbol"
#elif __APPLE__
            "Apple Color Emoji"
#else // Linux
            "Noto Color Emoji", "EmojiOne Color", "Symbola"
#endif
        };
        return v;
    }

    // возвращает первый валидный из списка or системный по умолчанию
    static wxFont GetBestFont(const std::vector<std::string>& candidates,
        int pointSize = 10) {
        for (auto& face : candidates) {
            wxFont f(pointSize, wxFONTFAMILY_DEFAULT,
                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                false, face);
            if (f.IsOk()) return f;
        }
        return wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    }

    static wxFont GetGuiFont(int sz = 12) {
        return GetBestFont(GuiFontCandidates(), sz);
    }
    static wxFont GetEmojiFont(int sz = 12) {
        return GetBestFont(EmojiFontCandidates(), sz);
    }
};

static std::string to_utf8(const wxString& str) {
    wxScopedCharBuffer buffer = str.ToUTF8();
    return std::string(buffer.data(), buffer.length());
}

namespace gui {

static void CreateErrorBox(const std::string& msg) {
    wxMessageBox(wxString::FromUTF8(msg),
        wxString::FromUTF8("Ошибка"), wxICON_ERROR);
}

static void CreateWrongInputBox(const std::string& msg) {
    wxMessageBox(wxString::FromUTF8(msg + " содержит недопустимые символы.\n"
        "Разрешены только: цифры, английские буквы, и символы: !#%?*()_-+=<>"),
        wxString::FromUTF8("Ошибка ввода"), wxICON_WARNING);
}

static void CreateInfoBox(const std::string& msg) {
    wxMessageBox(wxString::FromUTF8(msg),
        wxString::FromUTF8("Предупреждение"), wxICON_WARNING);
}

}// namespace gui
