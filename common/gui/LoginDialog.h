#pragma once
#include <wx/wx.h>
#include <wx/listbox.h>
#include <wx/valtext.h>
#include <vector>
#include "ssl.h"
#include "RegisterDialog.h"

namespace gui{

class LoginDialog : public wxDialog {
public: 
    LoginDialog(wxWindow* parent);
    ~LoginDialog() {
        if (network_client_) {
            network_client_.reset();
        }
    }

    wxString GetUsername() const { return username_field_->GetValue(); }
    wxString GetPassword() const { return password_field_->GetValue(); }
    wxString GetServer() const { return server_list_->GetStringSelection(); }
    bool RememberMe() const { return remember_check_->GetValue(); }

private:
    wxTextCtrl* username_field_;
    wxTextCtrl* password_field_;
    wxCheckBox* remember_check_;
    wxListBox* server_list_;
    wxButton* login_button_;
    wxButton* register_button_;
    wxButton* cancel_button_;
    wxButton* add_button_;
    wxButton* delete_button_;
    wxButton* update_button_;
    bool m_remembered_; 
    wxString m_remembered_username_;
    std::unique_ptr<Client> network_client_;
    wxString last_connected_server_;

    void ConstructInterface();
    void OnLogin(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnRegister(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnAddServer(wxCommandEvent& event);
    void OnDeleteServer(wxCommandEvent& event);
    void OnUpdateServers(wxCommandEvent& event);
    bool ValidateServerFormat(const wxString& server);
    void LoadConfig();               
    void SaveConfig();               
    void UpdateUserSection(std::ofstream& file);
    void ConnetToSelectedServer();

    bool HandleNetworkMessage(const std::string& json_msg);
};

}//end namespace gui