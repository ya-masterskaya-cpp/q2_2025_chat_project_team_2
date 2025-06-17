#pragma once
#include <wx/wx.h>
#include <wx/listbox.h>
#include <vector>
#include <set>
#include "config.h"

namespace gui{

class wxStringClientData : public wxClientData {
public:
    wxStringClientData(const wxString& value, bool disabled) 
        : value_(value), disabled_(disabled) {}
        
    const wxString& GetValue() const { return value_; }
    bool IsDisabled() const { return disabled_; }
    
private:
    wxString value_;
    bool disabled_;
};

class ListSelectionDialog : public wxDialog {
public:
    ListSelectionDialog(wxWindow* parent,
        const wxString& title,
        const std::vector<std::string>& items,
        const std::set<wxString>& existing_items);

    wxString GetSelectedItem() const { return list_box_->GetStringSelection(); }

private:
    wxListBox* list_box_;
    wxButton* add_button_;
    wxButton* cancel_button_;

    void OnAdd(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnItemSelected(wxCommandEvent& event);
};

} //end namespace gui