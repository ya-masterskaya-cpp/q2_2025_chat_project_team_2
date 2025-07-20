#pragma once

namespace sql {

    static const char* CHANGE_USER_NAME = R"sql(
        UPDATE users
            SET name = ?
            WHERE login = ?;
    )sql";

    static const char* CHANGE_ROOM_NAME = R"sql(
        UPDATE rooms
            SET room = ?
            WHERE room = ?;
    )sql";

    static const char* GET_USER_DATA = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id
        WHERE u.login = ?;
    )sql";

    static const char* GET_ALL_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id;
    )sql";

    static const char* GET_ACTIVE_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id
        WHERE u.is_deleted = false;
    )sql";

    static const char* GET_DELETED_USERS = R"sql(
        SELECT 
            u.login, 
            u.name, 
            u.password_hash, 
            r.role, 
            u.is_deleted, 
            u.unixtime 
        FROM users as u
        JOIN roles AS r ON u.roles_id = r.roles_id
        WHERE u.is_deleted = true;
    )sql";

    static const char* GET_ROOM_ACTIVE_USERS = R"sql(
    SELECT
        u.login,
        u.name,
        u.password_hash,
        r.role,
        u.is_deleted,
        u.unixtime AS user_registration_time
        FROM users u
        JOIN roles r ON u.roles_id = r.roles_id
        JOIN user_rooms ur ON u.users_id = ur.users_id
        JOIN rooms rm ON ur.rooms_id = rm.rooms_id
        WHERE rm.room = ? --Параметр имени комнаты
        AND u.is_deleted = 0;  --Только активные пользователи
    )sql";

    static const char* GET_USER_ROOMS = R"sql(
    SELECT
        r.room
        FROM rooms AS r
        JOIN user_rooms AS ur ON r.rooms_id = ur.rooms_id
        JOIN users AS u ON u.users_id = ur.users_id
        WHERE u.login = ?;
    )sql";

    static const char* GET_ALL_PAIR_ROOMS_AND_USERS = R"sql(
    SELECT
        r.room,
        u.login
        FROM user_rooms AS ur
        JOIN rooms AS r ON ur.rooms_id = r.rooms_id 
        JOIN users AS u ON ur.users_id = u.users_id;
    )sql";

    static const char* CREATE_USER = R"sql(
        INSERT OR IGNORE INTO users(login, name, password_hash, roles_id, is_deleted, unixtime)
            VALUES(? , ? , ? , (SELECT roles_id FROM roles WHERE role = ? ), ?, ? );
    )sql";

    static const char* DELETE_USER = R"sql(
        DELETE FROM users AS u
        WHERE u.login = ?
        AND NOT EXISTS (
            SELECT 1
            FROM user_rooms AS ur
            WHERE ur.users_id = u.users_id
        );
    )sql";

    static const char* DELETE_DELETED_USER_WITHOUT_ROOM = R"sql(
        DELETE FROM users
            WHERE is_deleted = 1
            AND NOT EXISTS(
                SELECT 1
                FROM user_rooms
                WHERE user_rooms.users_id = users.users_id
            );
    )sql";

    static const char* ROOM_USERS_SQL = R"sql(
        SELECT
            u.login,
            u.name,
            u.password_hash,
            ro.role,
            u.is_deleted
        FROM users AS u
        JOIN user_rooms AS ur ON u.users_id = ur.users_id
        JOIN rooms      AS r  ON ur.rooms_id = r.rooms_id
        JOIN roles      AS ro ON u.roles_id = ro.roles_id -- Добавляем JOIN с roles
        WHERE r.room = ?;
    )sql";

    static const char* ADD_USER_TO_ROOM = R"sql(
        INSERT OR IGNORE INTO user_rooms (users_id, rooms_id)
        VALUES (
            (SELECT users_id FROM users WHERE login = ?),
            (SELECT rooms_id FROM rooms WHERE room = ?)
        );
    )sql";

    static const char* DELETE_USER_FROM_ROOM = R"sql(
        DELETE FROM user_rooms
        WHERE users_id = (SELECT users_id FROM users WHERE login = ?)
            AND rooms_id = (SELECT rooms_id FROM rooms WHERE room = ?);
    )sql";

    static const char* GET_RECENT_ROOM_MESSAGES = R"sql(
        SELECT 
            m.message,
            u.login       AS user_login,
            r.room        AS room_name,
            m.unixtime
        FROM messages AS m
        JOIN users AS u   ON m.users_id = u.users_id
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?
        ORDER BY m.unixtime DESC
        LIMIT 50;
    )sql";

    static const char* GET_MESSAGES_ROOM_AFTER = R"sql(
        SELECT 
            m.message,
            u.login       AS user_login,
            r.room        AS room_name,
            m.unixtime
        FROM messages AS m
        JOIN users AS u   ON m.users_id = u.users_id
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?
          AND m.unixtime < ?
        ORDER BY m.unixtime DESC
        LIMIT 50;
    )sql";

    static const char* GET_COUNT_ROOM_MESSAGES = R"sql(
        SELECT COUNT(messages_id)
        FROM messages AS m
        JOIN rooms AS r   ON m.rooms_id = r.rooms_id
        WHERE r.room = ?;
    )sql";

    static const char* INSERT_MESSAGE_TO_DB = R"sql(
        INSERT INTO messages(
            message,
            unixtime,
            users_id,
            rooms_id,
            date,
            time
        )
            VALUES(
                ?, ?,
                (SELECT users_id FROM users WHERE login = ?),
                (SELECT rooms_id FROM rooms WHERE room = ?),
                ?, ?
            );
    )sql";

    static const char* INIT_SQL = R"sql(
        CREATE TABLE IF NOT EXISTS metadata (
            key TEXT PRIMARY KEY, 
            value TEXT
        );
        INSERT OR IGNORE INTO metadata (key, value) VALUES ('schema_version', '1');

        CREATE TABLE IF NOT EXISTS roles (
            roles_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            role TEXT UNIQUE NOT NULL
        );
        INSERT OR IGNORE INTO roles(role) VALUES ('admin'), ('user');

        CREATE TABLE IF NOT EXISTS rooms (
            rooms_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            room TEXT UNIQUE NOT NULL,
            unixtime INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_rooms_room ON rooms(room);

        CREATE TABLE IF NOT EXISTS users ( 
            users_id INTEGER PRIMARY KEY AUTOINCREMENT, 
            login TEXT UNIQUE NOT NULL, 
            name TEXT NOT NULL, 
            password_hash TEXT NOT NULL,
            roles_id INTEGER NOT NULL REFERENCES roles(roles_id),
            is_deleted BOOLEAN NOT NULL DEFAULT 0,
            unixtime INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);

        CREATE TABLE IF NOT EXISTS messages (
            messages_id INTEGER PRIMARY KEY AUTOINCREMENT,
            message TEXT NOT NULL,
            unixtime INTEGER NOT NULL,
            users_id INTEGER NOT NULL REFERENCES users(users_id),
            rooms_id INTEGER NOT NULL REFERENCES rooms(rooms_id) ON DELETE CASCADE,
            date TEXT NOT NULL,
            time TEXT NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_messages_room_user ON messages(rooms_id, users_id);
        CREATE INDEX IF NOT EXISTS idx_room_time ON messages(rooms_id, unixtime DESC);
        
        CREATE TABLE IF NOT EXISTS user_rooms (
            user_rooms_id INTEGER PRIMARY KEY AUTOINCREMENT,
            users_id INTEGER NOT NULL REFERENCES users(users_id) ON DELETE CASCADE,
            rooms_id INTEGER NOT NULL REFERENCES rooms(rooms_id) ON DELETE CASCADE,
            UNIQUE(users_id, rooms_id)
        );
    )sql";

} // sql