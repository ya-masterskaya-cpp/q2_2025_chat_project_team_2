#include "ListSelectionDialog.h"

namespace gui {

ListSelectionDialog::ListSelectionDialog(wxWindow* parent,
    const wxString& title,
    const std::set<std::string>& items,
    const std::set<wxString>& existing_items)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(400, 500))
{
    SetFont(DEFAULT_FONT);

    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

    // Список элементов
    list_box_ = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_NEEDED_SB);

    // Заполняем список и отмечаем существующие элементы
    for (const auto& item : items) {
        if (existing_items.find(item) != existing_items.end()) {
            // Элемент уже существует - добавляем с пометкой
            list_box_->Append(item + " (уже добавлено)");
            list_box_->SetClientObject(list_box_->GetCount() - 1, new wxStringClientData(item, true));
        }
        else {
            // Новый элемент
            list_box_->Append(item);
            list_box_->SetClientObject(list_box_->GetCount() - 1, new wxStringClientData(item, false));
        }
    }

    main_sizer->Add(list_box_, 1, wxEXPAND | wxALL, 10);

    // Кнопки
    wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer();

    cancel_button_ = new wxButton(panel, wxID_CANCEL, "Отменить");
    add_button_ = new wxButton(panel, wxID_OK, "Добавить");
    add_button_->Enable(false); // Изначально неактивна

    button_sizer->Add(add_button_, 0, wxRIGHT, 10);
    button_sizer->Add(cancel_button_, 0);

    main_sizer->Add(button_sizer, 0, wxEXPAND | wxBOTTOM | wxRIGHT | wxLEFT, 10);

    panel->SetSizer(main_sizer);

    // Биндинг событий
    add_button_->Bind(wxEVT_BUTTON, &ListSelectionDialog::OnAdd, this);
    cancel_button_->Bind(wxEVT_BUTTON, &ListSelectionDialog::OnCancel, this);
    list_box_->Bind(wxEVT_LISTBOX, &ListSelectionDialog::OnItemSelected, this);

    Center();
}

void ListSelectionDialog::OnAdd(wxCommandEvent& event) {
    EndModal(wxID_OK);
}

void ListSelectionDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

void ListSelectionDialog::OnItemSelected(wxCommandEvent& event) {
    int selection = list_box_->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxClientData* data = list_box_->GetClientObject(selection);
        if (auto string_data = dynamic_cast<wxStringClientData*>(data)) {
            // Активируем кнопку только для элементов, которые еще не добавлены
            add_button_->Enable(!string_data->IsDisabled());
        }
    }
}

} // namespace gui