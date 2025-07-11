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

wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl) {
    wxString result;
    wxString full_text = ctrl->GetValue();

    // Если текст пустой, возвращаем пустую строку
    if (full_text.IsEmpty())
        return result;

    while (!full_text.IsEmpty() && (full_text.Last() == '\n' || full_text.Last() == '\r')) {
        full_text.RemoveLast();
    }

    // Создаем временный буфер для накопления форматированных фрагментов
    wxString current_fragment;
    wxRichTextAttr current_style;
    bool has_current_style = false;

    // Перебираем все символы в тексте
    for (long pos = 0; pos < full_text.length(); pos++) {
        wxRichTextAttr style;
        bool hasStyle = ctrl->GetStyle(pos, style);

        // Если стиль изменился или это первый символ
        if (!has_current_style || !hasStyle || !style.EqPartial(current_style)) {
            // Добавляем предыдущий фрагмент с форматированием
            if (!current_fragment.IsEmpty()) {
                // Добавляем открывающие теги
                if (current_style.GetFontWeight() == wxFONTWEIGHT_BOLD)
                    result += "[b]";
                if (current_style.GetFontStyle() == wxFONTSTYLE_ITALIC)
                    result += "[i]";
                if (current_style.GetFontUnderlined())
                    result += "[u]";

                // Экранируем и добавляем текст фрагмента
                wxString escaped = current_fragment;
                escaped.Replace("[", "\\[");
                escaped.Replace("]", "\\]");
                escaped.Replace("\\", "\\\\");
                result += escaped;

                // Добавляем закрывающие теги
                if (current_style.GetFontUnderlined())
                    result += "[/u]";
                if (current_style.GetFontStyle() == wxFONTSTYLE_ITALIC)
                    result += "[/i]";
                if (current_style.GetFontWeight() == wxFONTWEIGHT_BOLD)
                    result += "[/b]";

                current_fragment.clear();
            }

            // Обновляем текущий стиль
            if (hasStyle) {
                current_style = style;
                has_current_style = true;
            }
            else {
                has_current_style = false;
            }
        }

        // Добавляем текущий символ в накапливаемый фрагмент
        current_fragment += full_text[pos];
    }

    // Добавляем последний фрагмент
    if (!current_fragment.IsEmpty()) {
        // Добавляем открывающие теги
        if (has_current_style) {
            if (current_style.GetFontWeight() == wxFONTWEIGHT_BOLD)
                result += "[b]";
            if (current_style.GetFontStyle() == wxFONTSTYLE_ITALIC)
                result += "[i]";
            if (current_style.GetFontUnderlined())
                result += "[u]";
        }

        // Экранируем и добавляем текст фрагмента
        wxString escaped = current_fragment;
        escaped.Replace("[", "\\[");
        escaped.Replace("]", "\\]");
        escaped.Replace("\\", "\\\\");
        result += escaped;

        // Добавляем закрывающие теги
        if (has_current_style) {
            if (current_style.GetFontUnderlined())
                result += "[/u]";
            if (current_style.GetFontStyle() == wxFONTSTYLE_ITALIC)
                result += "[/i]";
            if (current_style.GetFontWeight() == wxFONTWEIGHT_BOLD)
                result += "[/b]";
        }
    }

    return result;
}

void ParseBBCode(const wxString& text, wxRichTextCtrl* ctrl) {
    // Очистка стиля
    wxRichTextAttr attr;
    ctrl->SetDefaultStyle(attr);

    // Простой парсер с поддержкой вложенности
    size_t pos = 0;
    std::vector<wxString> tagStack;

    while (pos < text.length()) {
        if (text[pos] == '[' && pos + 1 < text.length()) {
            // Обработка тегов
            size_t endTag = text.find(']', pos);
            if (endTag != wxString::npos) {
                wxString tag = text.SubString(pos + 1, endTag - 1);
                pos = endTag + 1;

                if (tag.StartsWith("/")) {
                    // Закрывающий тег
                    if (!tagStack.empty()) {
                        tagStack.pop_back();
                    }

                    // Обновляем стиль на основе оставшихся тегов
                    attr = wxRichTextAttr();
                    for (const wxString& openTag : tagStack) {
                        ApplyTag(openTag, attr);
                    }
                    ctrl->SetDefaultStyle(attr);
                }
                else {
                    // Открывающий тег
                    tagStack.push_back(tag);
                    ApplyTag(tag, attr);
                    ctrl->SetDefaultStyle(attr);
                }
                continue;
            }
        }

        // Обработка экранирования
        if (text[pos] == '\\' && pos + 1 < text.length()) {
            ctrl->WriteText(wxString(text[pos + 1]));
            pos += 2;
        }
        else {
            ctrl->WriteText(wxString(text[pos]));
            pos++;
        }
    }
}

void ApplyTag(const wxString& tag, wxRichTextAttr& attr) {
    if (tag == "b") {
        attr.SetFontWeight(wxFONTWEIGHT_BOLD);
    }
    else if (tag == "i") {
        attr.SetFontStyle(wxFONTSTYLE_ITALIC);
    }
    else if (tag == "u") {
        attr.SetFontUnderlined(true);
    }
}

} //end namespace bbcode
} //end namespace gui