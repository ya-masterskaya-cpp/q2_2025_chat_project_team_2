#pragma once
#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/font.h>
#include <vector>
#include <map>
#include <config.h>

namespace gui {
namespace bbcode {

    // Структура одного смайлика
    struct Smiley {
        wxString emoji;         // Unicode символ смайла
        wxString bbcode_tag;    // Тег BBCode (например, "[smile]")
        wxString image_path;    // Путь к изображению (для будущего расширения)
        wxString description;   // Описание для всплывающей подсказки
    };
    //список всех смайлов
    std::vector<Smiley> GetSmileys();

    //преобразование richtext в BBCode с экранированием символов
    wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl);

    //обратное преобразование BBCode (стили и смайлы) в rich text control
    void ParseBBCode(const wxString& text, wxRichTextCtrl* display_field);

} //end namespace bbcode
} //end namespace gui