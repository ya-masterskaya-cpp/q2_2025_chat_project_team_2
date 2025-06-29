#pragma once
#include "string"
#include <wx/settings.h>
#include <wx/font.h>
#include <wx/fontenum.h>
#include <vector>

const std::string MAIN_ROOM_NAME = "general";
const std::string SYSTEM_SENDER_NAME = "System";
const wxFont DEFAULT_FONT_SECOND(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Segoe UI Emoji");
const wxFont DEFAULT_FONT(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial Unicode MS");
const std::string DEFAULT_SERVER = "127.0.0.1";
const std::string CLIENT_FILE_CONFIG = "/irc_chat/client_config/chat_servers.txt"; //as default to user_home_dir + \AppData\Roaming
const int MAX_MESSAGE_LENGTH = 512;
const std::string DEF_SERVER = "127.0.0.1";
const std::string CLIENT_FIRST_PORT = "9003";
const std::string CLIENT_SECOND_PORT = "9002";

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

static std::string NormalizeRoomName(const std::string& name) {
    // Преобразуем в wxString для корректной работы с UTF-8
    wxString wx_name = wxString::FromUTF8(name);

    // Удаляем начальные/конечные пробелы
    wx_name.Trim(true).Trim(false);

    // Возвращаем в UTF-8
    return to_utf8(wx_name);
}
