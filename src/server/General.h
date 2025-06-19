#pragma once
#include <string>

const int GENERAL = 101;
const int LOGIN = 111;
const int CHANGE_NAME = 112;
const int CREATE_ROOM = 113;
const int ENTER_ROOM = 114;
const int ASK_ROOMS = 115;
const int LEAVE_ROOM = 116;
const int LEAVE_CHAT = 117;

const std::string USER_EXISTS = "user exists";
const std::string NAME_EXISTS = "name exists";
const std::string ROOM_EXISTS = "room exists";
const std::string NO_ROOM = "room does not exist";
const std::string ENTER_TWICE = "user already entered the room";
const std::string LEAVE_TWICE = "user is not in the room";

const std::string LOG_FILE = "server_logs.txt";
