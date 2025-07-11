#pragma once
#include <wx/wx.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/font.h>
#include <vector>
#include <map>
#include <config.h>

namespace gui {
namespace bbcode {

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

void ApplyTag(const wxString& tag, wxRichTextAttr& attr);

} //end namespace bbcode
} //end namespace gui