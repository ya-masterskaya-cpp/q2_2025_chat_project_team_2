#pragma once
#include <vector>
#include <wx/string.h>
#include <wx/filename.h>
#include "config.h"

namespace util {

class ConfigManager{
public:
    ConfigManager();

    void Load();
    void Save() const;

    const std::vector<wxString>& GetServers() const { return servers_; }
    void SetServers(const std::vector<wxString>& servers) { servers_ = servers; }

    bool GetRememberMe() const { return remember_me_; }
    void SetRememberMe(bool remember) { remember_me_ = remember; }

    const wxString& GetRememberedUsername() const { return remembered_username_; }
    void SetRememberedUsername(const wxString& username) { remembered_username_ = username; }

    static wxString GetConfigFilePath();

private:
    std::vector<wxString> servers_;
    bool remember_me_ = false;
    wxString remembered_username_;

    void ParseConfigFile(std::ifstream& file);
    void HandleServerSection(const wxString& line);
    void HandleUserSection(const wxString& line);
};

}// namespace util
