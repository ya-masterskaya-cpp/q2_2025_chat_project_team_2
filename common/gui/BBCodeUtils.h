#pragma once
#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/font.h>

namespace gui {
namespace bbcode {

    // Структура одного смайлика
    struct Smiley {
        wxString code;
        wxString emoji;
    };
    //список всех смайлов
    std::vector<Smiley> GetSmileys();

    //преобразование richtext в BBCode с экранированием символов
    wxString ConvertRichTextToBBCode(wxRichTextCtrl* ctrl);

    //обратное преобразование BBCode (стили и смайлы) в rich text control
    void ParseBBCode(const wxString& text, wxRichTextCtrl* display_field);

} //end namespace bbcode
} //end namespace gui