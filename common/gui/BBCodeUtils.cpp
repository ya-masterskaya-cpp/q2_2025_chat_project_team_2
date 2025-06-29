#include "BBCodeUtils.h"

namespace gui {
namespace bbcode {

//std::vector<Smiley> GetSmileys() {
//    return {
//        { ":smile:", wxString::FromUTF8("😊") },
//        { ":laugh:", wxString::FromUTF8("😂") },
//        { ":heart:", wxString::FromUTF8("❤️") },
//        { ":thumbup:", wxString::FromUTF8("👍") },
//        { ":wave:", wxString::FromUTF8("👋") },
//        { ":party:", wxString::FromUTF8("🎉") },
//        { ":thinking:", wxString::FromUTF8("🤔") },
//        { ":cool:", wxString::FromUTF8("😎") },
//        { ":rofl:", wxString::FromUTF8("🤣") },
//        { ":love:", wxString::FromUTF8("😍") },
//        { ":cry:", wxString::FromUTF8("😢") },
//        { ":angry:", wxString::FromUTF8("😠") },
//        { ":clap:", wxString::FromUTF8("👏" )},
//        { ":pray:", wxString::FromUTF8("🙏") },
//        { ":strong:", wxString::FromUTF8("💪") },
//        { ":eyes:", wxString::FromUTF8("👀") },
//        { ":target:", wxString::FromUTF8("🎯") },
//        { ":fire:", wxString::FromUTF8("🔥") },
//        { ":rainbow:", wxString::FromUTF8("🌈") },
//        { ":star:", wxString::FromUTF8("⭐") }
//    };
//}

std::vector<Smiley> GetSmileys() {
    return {
        { wxString::FromUTF8("😊"), "[smile]", "", wxString::FromUTF8("Улыбка") },
        { wxString::FromUTF8("😂"), "[laugh]", "", wxString::FromUTF8("Смех") },
        { wxString::FromUTF8("😢"), "[cry]", "", wxString::FromUTF8("Плач") },
        { wxString::FromUTF8("😠"), "[angry]", "", wxString::FromUTF8("Злость") },
        { wxString::FromUTF8("😍"), "[love]", "", wxString::FromUTF8("Влюбленность") },
        { wxString::FromUTF8("😎"), "[cool]", "", wxString::FromUTF8("Круто") },
        { wxString::FromUTF8("🤔"), "[think]", "", wxString::FromUTF8("Размышление") },
        { wxString::FromUTF8("👍"), "[thumbsup]", "", wxString::FromUTF8("Одобрение") },
        { wxString::FromUTF8("👎"), "[thumbsdown]", "", wxString::FromUTF8("Неодобрение") },
        { wxString::FromUTF8("❤️"), "[heart]", "", wxString::FromUTF8("Сердце") },
        { wxString::FromUTF8("🎉"), "[party]", "", wxString::FromUTF8("Праздник") },
        { wxString::FromUTF8("🔥"), "[fire]", "", wxString::FromUTF8("Огонь") },
        // Добавьте больше смайлов по необходимости
    };
}

//currect
wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl) {
    wxString result;
    wxRichTextBuffer* buffer = &ctrl->GetBuffer();

    if (!buffer || ctrl->IsEmpty()) return result;

    // Итератор по всем элементам буфера
    wxRichTextObjectList::compatibility_iterator node = buffer->GetChildren().GetFirst();
    while (node) {
        if (wxRichTextParagraph* para = wxDynamicCast(node->GetData(), wxRichTextParagraph)) {
            // Обрабатываем все дочерние элементы параграфа
            wxRichTextObjectList::compatibility_iterator child_node = para->GetChildren().GetFirst();
            while (child_node) {
                wxRichTextObject* obj = child_node->GetData();
                const wxRichTextRange& range = obj->GetRange();

                // Получаем текст для диапазона
                wxString text_fragment = ctrl->GetRange(range.GetStart(), range.GetEnd());

                // Пропускаем пустые фрагменты (например, пустые строки)
                if (!text_fragment.IsEmpty()) {
                    const wxRichTextAttr& attr = obj->GetAttributes();

                    // Открываем теги стилей
                    if (attr.GetFontWeight() == wxFONTWEIGHT_BOLD) result += "[b]";
                    if (attr.GetFontStyle() == wxFONTSTYLE_ITALIC) result += "[i]";
                    if (attr.GetFontUnderlined()) result += "[u]";

                    // Экранирование специальных символов
                    text_fragment.Replace("[", "\\[");
                    text_fragment.Replace("]", "\\]");
                    text_fragment.Replace("\\", "\\\\");

                    result += text_fragment;

                    // Закрываем теги
                    if (attr.GetFontUnderlined()) result += "[/u]";
                    if (attr.GetFontStyle() == wxFONTSTYLE_ITALIC) result += "[/i]";
                    if (attr.GetFontWeight() == wxFONTWEIGHT_BOLD) result += "[/b]";
                }
                child_node = child_node->GetNext();
            }

            // Добавляем перенос строки между параграфами
            if (node->GetNext()) {
                result += "\n";
            }
        }
        node = node->GetNext();
    }

    // Убедимся, что обработан весь текст
    const wxString full_text = ctrl->GetValue();
    if (result.length() < full_text.length()) {
        wxString remaining = full_text.Mid(result.length());
        remaining.Replace("[", "\\[");
        remaining.Replace("]", "\\]");
        remaining.Replace("\\", "\\\\");
        result += remaining;
    }

    return result;
}

void ParseBBCode(const wxString& text, wxRichTextCtrl* display_field) {
    // Подготовка карты смайлов: [тег] -> эмодзи
    auto smileys = GetSmileys();
    std::unordered_map<wxString, wxString> smiley_tag_map;
    for (const auto& s : smileys) {
        smiley_tag_map[s.bbcode_tag.Lower()] = s.emoji;
    }

    // Состояние стилей
    bool in_bold = false;
    bool in_italic = false;
    bool in_underline = false;

    size_t i = 0;
    const size_t len = text.length();
    wxString text_buffer;

    // Функция для записи буфера с текущими стилями
    auto write_buffer = [&]() {
        if (!text_buffer.IsEmpty()) {
            if (in_bold) display_field->BeginBold();
            if (in_italic) display_field->BeginItalic();
            if (in_underline) display_field->BeginUnderline();

            display_field->WriteText(text_buffer);

            if (in_underline) display_field->EndUnderline();
            if (in_italic) display_field->EndItalic();
            if (in_bold) display_field->EndBold();

            text_buffer.clear();
        }
        };

    while (i < len) {
        // Обработка экранированных символов
        if (text[i] == '\\' && i + 1 < len) {
            wxChar next_char = text[i + 1];
            if (next_char == '[' || next_char == ']' || next_char == '\\') {
                text_buffer += next_char;
                i += 2;
                continue;
            }
        }

        // Обработка тегов
        if (text[i] == '[' && i + 1 < len) {
            bool tag_processed = false;

            // Закрывающие теги [/]
            if (i + 3 < len && text[i + 1] == '/') {
                wxString tag = text.SubString(i + 2, i + 3).Lower();
                if (tag == "b]" && in_bold) {
                    write_buffer();
                    in_bold = false;
                    i += 4;
                    tag_processed = true;
                }
                else if (tag == "i]" && in_italic) {
                    write_buffer();
                    in_italic = false;
                    i += 4;
                    tag_processed = true;
                }
                else if (tag == "u]" && in_underline) {
                    write_buffer();
                    in_underline = false;
                    i += 4;
                    tag_processed = true;
                }
            }
            // Открывающие теги
            else if (i + 2 < len) {
                wxString tag = text.SubString(i + 1, i + 2).Lower();
                if (tag == "b]") {
                    write_buffer();
                    in_bold = true;
                    i += 3;
                    tag_processed = true;
                }
                else if (tag == "i]") {
                    write_buffer();
                    in_italic = true;
                    i += 3;
                    tag_processed = true;
                }
                else if (tag == "u]") {
                    write_buffer();
                    in_underline = true;
                    i += 3;
                    tag_processed = true;
                }
            }

            // Обработка смайлов если не обработан стилевой тег
            if (!tag_processed) {
                // Ищем закрывающую скобку
                size_t end_pos = i + 1;
                while (end_pos < len && end_pos < i + 20) {
                    if (text[end_pos] == ']') break;
                    end_pos++;
                }

                if (end_pos < len && text[end_pos] == ']') {
                    wxString full_tag = text.SubString(i, end_pos);
                    wxString tag_lower = full_tag.Lower();

                    // Проверяем есть ли такой тег в смайлах
                    auto it = smiley_tag_map.find(tag_lower);
                    if (it != smiley_tag_map.end()) {
                        write_buffer();
                        display_field->WriteText(it->second);
                        i = end_pos + 1;
                        continue;
                    }
                }
            }
        }

        // Обычный текст
        if (i < len) {
            text_buffer += text[i];
            i++;
        }
    }

    // Запись оставшегося текста
    write_buffer();
}

} //end namespace bbcode
} //end namespace gui