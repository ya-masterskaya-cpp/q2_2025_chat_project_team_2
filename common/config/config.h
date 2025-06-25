#pragma once
#include "string"
#include <wx/font.h>

const std::string MAIN_ROOM_NAME = "general";
const std::string SYSTEM_SENDER_NAME = "System";
const wxFont DEFAULT_FONT_SECOND(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial Unicode MS");
const wxFont DEFAULT_FONT(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Arial Unicode MS");
const std::string DEFAULT_SERVER = "127.0.0.1";
const std::string CLIENT_FILE_CONFIG = "/irc_chat/client_config/chat_servers.txt"; //as default to user_home_dir + \AppData\Roaming
const int MAX_MESSAGE_LENGTH = 512;
const std::string DEF_SERVER = "127.0.0.1";
const std::string CLIENT_FIRST_PORT = "9003";
const std::string CLIENT_SECOND_PORT = "9002";

