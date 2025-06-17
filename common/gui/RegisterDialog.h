#pragma once
#include "wx/wx.h"
#include "config.h"

namespace gui {

class RegisterDialog : public wxDialog {
public:
    RegisterDialog(wxWindow* parent, const std::string& selected_sever);
    virtual ~RegisterDialog() = default;

    wxString GetUsername() const { return username_field_->GetValue(); }
    wxString GetPassword() const { return password_field_->GetValue(); }
    wxString GetConfirmPassword() const { return confirm_password_field_->GetValue(); }
    const std::string& GetServer() const { return server_; }

private:
    wxTextCtrl* username_field_;
    wxTextCtrl* password_field_;
    wxTextCtrl* confirm_password_field_;
    wxButton* cancel_button_;
    wxButton* register_button_;
    std::string server_;

    void ConstructInterface();
    void OnRegister(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
};

} //end namespace gui