#include "BBCodeUtils.h"

namespace gui {
namespace bbcode {

std::vector<Smiley> GetSmileys() {
    return {
        { ":smile:", "😊" },
        { ":laugh:", "😂" },
        { ":heart:", "❤️" },
        { ":thumbup:", "👍" },
        { ":wave:", "👋" },
        { ":party:", "🎉" },
        { ":thinking:", "🤔" },
        { ":cool:", "😎" },
        { ":rofl:", "🤣" },
        { ":love:", "😍" },
        { ":cry:", "😢" },
        { ":angry:", "😠" },
        { ":clap:", "👏" },
        { ":pray:", "🙏" },
        { ":strong:", "💪" },
        { ":eyes:", "👀" },
        { ":target:", "🎯" },
        { ":fire:", "🔥" },
        { ":rainbow:", "🌈" },
        { ":star:", "⭐" }
    };
}

wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl) {
    wxString result;
    long start = 0, end = ctrl->GetLastPosition();

    //флаги для отслеживания стиля
    bool is_bold = false;
    bool is_italic = false;
    bool is_underline = false;

    wxRichTextAttr prev_attr;
    if (end > 0) ctrl->GetStyle(0, prev_attr);

    for (long pos = start; pos < end; pos++) {
        wxRichTextAttr attr;
        ctrl->GetStyle(pos, attr);

        // Проверяем изменения стиля
        if (attr.GetFontWeight() != prev_attr.GetFontWeight()) {
            if (attr.GetFontWeight() == wxFONTWEIGHT_BOLD && !is_bold) {
                result += "[b]";
                is_bold = true;
            }
            else if (is_bold) {
                result += "[/b]";
                is_bold = false;
            }
        }

        if (attr.GetFontStyle() != prev_attr.GetFontStyle()) {
            if (attr.GetFontStyle() == wxFONTSTYLE_ITALIC && !is_italic) {
                result += "[i]";
                is_italic = true;
            }
            else if (is_italic) {
                result += "[/i]";
                is_italic = false;
            }
        }

        if (attr.GetFontUnderlined() != prev_attr.GetFontUnderlined()) {
            if (attr.GetFontUnderlined() && !is_underline) {
                result += "[u]";
                is_underline = true;
            }
            else if (is_underline) {
                result += "[/u]";
                is_underline = false;
            }
        }

        prev_attr = attr;

        // Обработка символа
        wxChar ch = ctrl->GetValue()[pos];
        if (ch == '[') {
            result += "\\[";
        }
        else if (ch == ']') {
            result += "\\]";
        }
        else if (ch == '\\') {
            result += "\\\\";
        }
        else {
            result += ch;
        }
    }

    // Закрываем все открытые теги
    if (is_underline) result += "[/u]";
    if (is_italic) result += "[/i]";
    if (is_bold) result += "[/b]";

    return result;
}

void ParseBBCode(const wxString& text, wxRichTextCtrl* display_field) {
    auto smileys = GetSmileys();
    std::unordered_map<wxString, wxString> smiley_map;
    for (const auto& s : smileys) smiley_map[s.code] = s.emoji;
        
    bool in_bold = false;
    bool in_italic = false;
    bool in_underline = false;

    wxRichTextAttr base_attr;
    base_attr.SetFontSize(10);


    for (size_t i = 0; i < text.length(); i++) {
        // Обработка тегов
        if (text[i] == '[' && i + 1 < text.length()) {
            // Тег [b]
            if (i + 3 <= text.length() && text.Mid(i, 3) == "[b]") {
                display_field->BeginBold();
                in_bold = true;
                i += 2; // Пропускаем 2 символа
                continue;
            }
            // Тег [/b]
            else if (i + 4 <= text.length() && text.Mid(i, 4) == "[/b]") {
                if (in_bold) display_field->EndBold();
                in_bold = false;
                i += 3;
                continue;
            }
            // Тег [i]
            else if (i + 3 <= text.length() && text.Mid(i, 3) == "[i]") {
                display_field->BeginItalic();
                in_italic = true;
                i += 2;
                continue;
            }
            // Тег [/i]
            else if (i + 4 <= text.length() && text.Mid(i, 4) == "[/i]") {
                if (in_italic) display_field->EndItalic();
                in_italic = false;
                i += 3;
                continue;
            }
            // Тег [u]
            else if (i + 3 <= text.length() && text.Mid(i, 3) == "[u]") {
                display_field->BeginUnderline();
                in_underline = true;
                i += 2;
                continue;
            }
            // Тег [/u]
            else if (i + 4 <= text.length() && text.Mid(i, 4) == "[/u]") {
                if (in_underline) display_field->EndUnderline();
                in_underline = false;
                i += 3;
                continue;
            }
        }

        // Обработка смайликов
        if (text[i] == ':' && i + 1 < text.length()) {
            size_t end_pos = text.find(':', i + 1);
            if (end_pos != wxString::npos) {
                wxString code = text.SubString(i, end_pos);
                if (smiley_map.find(code) != smiley_map.end()) {
                    display_field->WriteText(smiley_map[code]);
                    i = end_pos; // переход на конец
                    continue;
                }
            }
        }

        // Обработка экранированных символов
        if (text[i] == '\\' && i + 1 < text.length()) {
            wxChar next_char = text[i + 1];
            if (next_char == '[' || next_char == ']' || next_char == '\\') {
                display_field->WriteText(wxString(next_char));
                i++; // Пропускаем экранирующий символ
                continue;
            }
        }

        // Обычный текст
        display_field->WriteText(wxString(text[i]));
    }

    // Закрываем все открытые теги (на всякий случай)
    if (in_underline) display_field->EndUnderline();
    if (in_italic) display_field->EndItalic();
    if (in_bold) display_field->EndBold();
}

} //end namespace bbcode
} //end namespace gui