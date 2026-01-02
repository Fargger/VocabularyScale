-- --------------------------
-- 初始化程序数据库 vocab_system.db
-- --------------------------

DROP TABLE IF EXISTS users;

CREATE TABLE users (
    uuid TEXT PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    user_level INTEGER,
    class_name TEXT,
    student_num INTEGER,
    teacher_uuid TEXT
);

-- 密码 123456 对应的 hash 值是 2110877786
-- 分别插入用户等级为 0 1 2 的三名用户

INSERT INTO user (uuid, username, password_hash, user_level, class_name, student_num, teacher_uuid) VALUES
('1aa93924-5c6a-433f-b5a4-20e30917c22b', 'huarun', '2110877786', 0, '310', '3125004592', '50615c09-71e9-4bc7-875b-62718ec0873d'),
('50615c09-71e9-4bc7-875b-62718ec0873d', 'teacher1', '2110877786', 1, '1', NULL, NULL),
('238d3617-beea-46df-b2b6-b1f9d213f4d4', 'teacher2', '2110877786', 1, '2', NULL, NULL),
('179425c3-228e-49e4-96f2-7d77a094e171', 'lks', '2110877786', 2, '210','114514', '50615c09-71e9-4bc7-875b-62718ec0873d');