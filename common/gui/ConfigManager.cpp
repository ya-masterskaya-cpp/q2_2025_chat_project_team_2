#include "ConfigManager.h"
#include <fstream>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>

namespace util {

ConfigManager::ConfigManager() {
    Load();
}

void ConfigManager::Load() {
    servers_.clear();
    remember_me_ = false;
    remembered_username_.clear();

    wxString filename = GetConfigFilePath();
    if (!wxFileExists(filename)) return;

    std::ifstream file(filename.ToStdString());
    if (!file.is_open()) return;

    ParseConfigFile(file);
}

void ConfigManager::Save() const {
    wxString filename = GetConfigFilePath();
    wxFileName fn(filename);

    if (!fn.DirExists()) {
        fn.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }

    std::ofstream file(filename.ToStdString());
    if (!file) return;

    // Сохранение серверов
    file << "[servers]\n";
    for (const auto& server : servers_) {
        file << server.ToUTF8() << "\n";
    }

    // Сохранение пользовательских данных
    file << "\n[user]\n";
    file << "remember=" << (remember_me_ ? "1" : "0") << "\n";
    if (remember_me_ && !remembered_username_.empty()) {
        file << "username=" << remembered_username_.ToUTF8() << "\n";
    }
}

wxString ConfigManager::GetConfigFilePath() {
    return wxStandardPaths::Get().GetUserConfigDir() + CLIENT_FILE_CONFIG;
}

void ConfigManager::ParseConfigFile(std::ifstream& file) {
    std::string line;
    wxString current_section;

    while (std::getline(file, line)) {
        wxString wline = wxString::FromUTF8(line.c_str()).Trim().Trim(false);
        if (wline.IsEmpty()) continue;

        // Обработка секций
        if (wline.StartsWith("[") && wline.EndsWith("]")) {
            current_section = wline.Mid(1, wline.Length() - 2).Lower();
            continue;
        }

        // Обработка содержимого секций
        if (current_section == "servers") {
            HandleServerSection(wline);
        }
        else if (current_section == "user") {
            HandleUserSection(wline);
        }
    }
}

void ConfigManager::HandleServerSection(const wxString& line) {
    servers_.push_back(line);
}

void ConfigManager::HandleUserSection(const wxString& line) {
    int pos = line.Find('=');
    if (pos == wxNOT_FOUND) return;

    wxString key = line.Left(pos).Trim().Trim(false).Lower();
    wxString value = line.Mid(pos + 1).Trim().Trim(false);

    if (key == "remember") {
        remember_me_ = (value == "1");
    }
    else if (key == "username") {
        remembered_username_ = value;
    }
}


}// namespace util