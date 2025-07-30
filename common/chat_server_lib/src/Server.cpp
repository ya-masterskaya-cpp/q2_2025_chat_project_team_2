#include "Server.h"

void Server::run_server(db::DB& data_base) {
    logger_.logEvent("Server started");
    
    db_ = &data_base;
    for (auto& user : db_->GetAllUsers()) {
        users_passwords_[user.login] = user.password_hash;
    }
    db_->CreateRoom("general", utime::GetUnixTimeNs());

    members_who_wrote_ = move(db_->GetAllRoomWithRegisteredUsers());
    logger_.logEvent("Room membership cache loaded.");

    signals_.async_wait([this](const boost::system::error_code& /*ec*/, int /*signo*/) {
        logger_.logEvent("Shutdown signal received");
        is_running_ = false;
        ioc.stop();
        in_msg.finish();
        });

    std::thread(&Server::shuttle, this).detach();
    std::thread(&Server::client_accept, this).detach();

    ioc.run();

    logger_.logEvent("Server stopped");
}

void Server::sender(tcp::socket socket, MsgQueue* session) {
    websocket::stream<tcp::socket> ws(std::move(socket));
    ws.accept();
    ws.write(asio::buffer(
        MesBuilder(GENERAL).add("content", "hello").toString()
    ));
    if (!session->wait_on_queue([&](const json& j) {
        ws.write(asio::buffer(j.dump())); })
        ) {
        delete session;
    }
}

void Server::getter(tcp::socket socket, MsgQueue* session) {
    try {
        websocket::stream<tcp::socket> ws(std::move(socket));
        ws.accept();
        beast::flat_buffer buffer;
        bool finished = false;

        while (!finished && is_running_)
        {
            ws.read(buffer);
            json mes = json::parse(beast::buffers_to_string(buffer.data()));
            json ans;
            json res;
            ans["type"] = 0;
            int type = mes["type"];
            switch (type)
            {
            case REGISTER:
                ans = register_user(mes["user"], mes["password"]);
                break;
            case LOGIN:
                if (!check_login(mes["user"], mes["password"])) {
                    ans = make_err_answer(LOGIN, mes["user"], WRONG_LOGPASS);
                }
                else if (!logged_users_.count(mes["user"])) {
                    ans = add_user(session, mes["user"]);
                    ans["name"] = session->name;
                    in_msg.add(ask_users("general"));
                }
                else {
                    change_session(session, mes["user"]);
                }
                break;
            case CHANGE_NAME:
                ans = change_name(session, mes["name"]);
                break;
            case CREATE_ROOM:
                ans = create_room(session, mes["room"]);
                break;
            case ENTER_ROOM:
                res = enter_room(session, mes["room"]);
                session->add(res);
                if (res["answer"] == "OK") {
                    in_msg.add(ask_users(mes["room"]));
                }
                break;
            case ASK_ROOMS:
                ans = ask_rooms(session);
                break;
            case ASK_USERS:
                ans = ask_users(mes["room"]);
                break;
            case LEAVE_ROOM:
                ans = leave_room(session, mes["room"]);
                in_msg.add(ask_users(mes["room"]));
                break;
            case LEAVE_CHAT:
                remove_user(session);
                finished = true;
                break;
            case CHECK_LOGIN:
                ans = check_login(mes["user"], mes["password"]) ?
                    make_ok_answer(CHECK_LOGIN, mes["user"]) :
                    make_err_answer(CHECK_LOGIN, mes["user"], WRONG_LOGPASS);
                if (logged_users_.count(mes["user"]) && (*users_[logged_users_[mes["user"]]].begin())->is_online_)
                {
                    ans = make_err_answer(CHECK_LOGIN, mes["user"], USER_EXISTS);
                }
                break;
            case GENERAL:
                mes["user"] = session->name;
                mes["login"] = session->login;
                mes["server_timestamp"] = utime::GetUnixTimeNs();
                in_msg.add(mes);
                break;
            default:
                break;
            }
            int a = ans["type"];
            if (a) {
                session->add(ans);
            }
            buffer.consume(buffer.size());
        }
    }
    catch (...) {
        session->invalidate();
        logger_.logError("Client " + session->login + " disconnected");
    }
    session->finish();
}

void Server::client_accept() {
    tcp::acceptor in_acceptor(ioc, tcp::endpoint(tcp::v4(), in_port));
    tcp::acceptor out_acceptor(ioc, tcp::endpoint(tcp::v4(), out_port));
    while (is_running_) {
        MsgQueue* session = new MsgQueue;

        tcp::socket in_socket(ioc);
        in_acceptor.accept(in_socket);
        std::thread(&Server::getter, this, std::move(in_socket), session).detach();

        tcp::socket out_socket(ioc);
        out_acceptor.accept(out_socket);
        std::thread(&Server::sender, this, std::move(out_socket), session).detach();
    }
}

void Server::shuttle() {
    in_msg.wait_on_queue([&](const json& j) {

        if (j.contains("type") && j["type"] == GENERAL &&
            j.contains("login") && j.contains("room") &&
            j.contains("content") && j.contains("server_timestamp"))
        {
            const std::string& login = j["login"];
            const std::string& room_name = j["room"];
            const std::string& content = j["content"];
            int64_t timestamp = j["server_timestamp"];

            bool is_first_message = false;
            {
                std::lock_guard<std::mutex> lock(members_mutex_);
                auto& writers_in_this_room = members_who_wrote_[room_name];
                is_first_message = writers_in_this_room.insert(login).second;
            } 

            if (is_first_message) {
                logger_.logEvent("User '" + login + "' wrote first message in '" + room_name + "'. Creating DB link.");
                db_->AddUserToRoom(login, room_name);
            }

            if (!db_->InsertMessageToDB({ content, timestamp, login, room_name })) {
                logger_.logError("Failed to save message to DB for user " + login);
            }
        }
        for (auto ses : users_[j["room"]]) {
            ses->add(j);
        }
    });
}

json Server::make_ok_answer(int type, const std::string& what) {
    return MesBuilder(type).add("what", what).add("answer", "OK").j;
}

json Server::make_err_answer(int type, const std::string& what, const std::string& reason) {
    return MesBuilder(type).add("what", what).add("answer", "err").add("reason", reason).j;
}

bool Server::check_login(const std::string& login, const std::string& password) {
    logger_.logEvent("Query to check login: " + login + ".");

    auto it = users_passwords_.find(login);

    if (it != users_passwords_.end() && it->second == password) {
        return true;
    }

    logger_.logEvent("Login/password check failed for user: " + login);
    return false;
}

json Server::register_user(const std::string& login, const std::string& password) {
    if (users_passwords_.count(login) || db_->IsUser(login)) {
        logger_.logEvent("User (login: " + login + ") already exists.");
        return make_err_answer(REGISTER, login, LOGIN_EXISTS);
    }
    users_passwords_[login] = password;
    logger_.logEvent("User (login: " + login + ") is created.");

    if (db_->CreateUser({ login, login, password,"user", false, utime::GetUnixTimeNs() })) {
        db_->AddUserToRoom(login, "general");
        logger_.logEvent("User (login: " + login + ") saved to Data Base");
    }
    else {
        logger_.logEvent("Error. User (login: " + login + ") don't saved to Data Base");
    }

    return make_ok_answer(REGISTER, login);
}

void Server::change_session(MsgQueue* session, const std::string& login) {
    MsgQueue* old_session = *users_[logged_users_[login]].begin();
    if (old_session->is_online_) {
        json ans = make_err_answer(LOGIN, login, USER_EXISTS);
        ans["name"] = session->name;
        session->add(ans);
    }
    else {
        for (auto& u : users_) {
            if (u.second.erase(old_session)) {
                u.second.insert(session);
            }
        }
        session->login = login;
        session->name = logged_users_[login];
        json ans = make_ok_answer(LOGIN, login);
        ans["name"] = session->name;
        session->add(ans);
        *session = *old_session;
        delete old_session;
        logger_.logEvent("Client " + login + " connected again");
    }
}

json Server::add_user(MsgQueue* session, const std::string& login) {
    auto user_data_opt = db_->GetUserData(login);
    if (user_data_opt) {
        auto& user_data = *user_data_opt;

        if (user_data.is_deleted) {
            logger_.logError("Attempt to login by a deleted user: " + login);
            return make_err_answer(LOGIN, login, "User account is disabled");
        }

        logged_users_[login] = user_data.name;
        users_[user_data.name].insert(session);
        users_["general"].insert(session);
        session->name = user_data.name;
        session->login = login;
        logger_.logEvent("Client " + login + " logged in");
        json response = make_ok_answer(LOGIN, login);
        response["name"] = user_data.name;
        return response;
    }

    logger_.logError("User " + login + " not found in DB, but passed check_login. Creating temporary session.");
    logged_users_[login] = login;
    users_[login].insert(session);
    users_["general"].insert(session);
    session->name = login;
    session->login = login;
    logger_.logEvent("Client " + login + " logged in");
    return make_ok_answer(LOGIN, login);
}

json Server::change_name(MsgQueue* session, const std::string& new_name) {
    if (users_.count(new_name)) {
        return make_err_answer(CHANGE_NAME, new_name, NAME_EXISTS);
    }
    std::string old_name = session->name;
    logged_users_[session->login] = new_name;
    users_.erase(old_name);
    users_[new_name].insert(session);
    session->name = new_name;
    in_msg.add(MesBuilder(INFO_NAME)
        .add("room", "general")
        .add("old name", old_name)
        .add("new_name", new_name).j);
    logger_.logEvent("Client " + session->login + " changed name to " + new_name);
    db_->ChangeUserName(session->login, new_name);
    return make_ok_answer(CHANGE_NAME, new_name);
}

json Server::ask_rooms(MsgQueue* session) {
    std::set<std::string> unique_room_names;
    for (auto& u : users_) {
        unique_room_names.insert(u.first);
    }

    const std::string& login = session->login;
    auto user_db_rooms = db_->GetUserRooms(login);

    for (const auto& room : user_db_rooms) {
        unique_room_names.insert(room);
    }
    std::vector<std::string> v (unique_room_names.begin(), unique_room_names.end());

    return MesBuilder(ASK_ROOMS).add("rooms", v).j;
}

json Server::ask_users(const std::string& room) {
    std::vector<std::string> v;
    if (!users_.count(room)) {
        return make_err_answer(ASK_USERS, room, NO_ROOM);
    }
    for (auto u : users_[room]) {
        v.push_back(u->name);
    }
    return MesBuilder(ASK_USERS).add("room", room).add("users", v).j;
}

void Server::remove_user(MsgQueue* session) {
    std::string user_name = session->name;
    for (auto& u : users_) {
        if (u.second.erase(session) > 0) {
            if (u.first != user_name) {
                in_msg.add(ask_users(u.first));
            }
        }
    }
    users_.erase(logged_users_[session->login]);
    logged_users_.erase(session->login);
    logger_.logEvent("Client " + session->login + " left chat");
}

json Server::create_room(MsgQueue* session, const std::string& room_name) {
    if (users_.count(room_name) || db_->IsRoom(room_name)) {
        return make_err_answer(CREATE_ROOM, room_name, ROOM_EXISTS);
    }
    const std::string login = session->login;

    users_.emplace(std::make_pair(room_name, std::unordered_set<MsgQueue*>()));
    logger_.logEvent("Room " + room_name + " created");

    if (db_->CreateRoom(room_name, utime::GetUnixTimeNs())) {
        db_->AddUserToRoom(login, room_name);
        logger_.logEvent("Room " + room_name + " saved to Data Base");
    }
    else {
        logger_.logEvent("Error. Room " + room_name + " don't saved to Data Base");
    }

    return make_ok_answer(CREATE_ROOM, room_name);
}

json Server::enter_room(MsgQueue* session, const std::string& room_name) {
    auto it = users_.find(room_name);

    if (it == users_.end()) {
        if (db_->IsRoom(room_name)) {
            users_.emplace(room_name, std::unordered_set<MsgQueue*>());
        }
        else {
            return make_err_answer(ENTER_ROOM, room_name, NO_ROOM);
        }
    }
    if (!users_.at(room_name).insert(session).second) {
        return make_err_answer(ENTER_ROOM, room_name, ENTER_TWICE);
    }

    return make_ok_answer(ENTER_ROOM, room_name);
}

json Server::leave_room(MsgQueue* session, const std::string& room_name) {
    if (!users_.count(room_name)) {
        return make_err_answer(LEAVE_ROOM, room_name, NO_ROOM);
    }
    if (!users_[room_name].erase(session)) {
        return make_err_answer(LEAVE_ROOM, room_name, LEAVE_TWICE);
    }
    return make_ok_answer(LEAVE_ROOM, room_name);
}