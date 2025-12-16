-- --------------------------
-- 初始化程序数据库 .data.db
-- --------------------------

DROP TABLE IF EXISTS usr;

CREATE TABLE usr (
    uuid VARCHAR(50) PRIMARY KEY,
    usr_name VARCHAR(30) NOT NULL,
    pswd_hash VARCHAR(100),
    usr_level INTEGER NOT NULL,
    class_name VARCHAR(20),
    num INTEGER NOT NULL,
    belong_to VARCHAR(50)
);

INSERT INTO usr (uuid, usr_name, usr_level, class_name, num, belong_to) VALUES
('1aa93924-5c6a-433f-b5a4-20e30917c22b', 'huarun', 0, NULL, 3125004592, NULL),
('a14dd09c-2c16-42d5-a2b8-7dca0c8b67bc', 'Lynn', 1, NULL, 1919810, NULL),
('179425c3-228e-49e4-96f2-7d77a094e171', 'lks', 2, 'suki', 114513, 'a14dd09c-2c16-42d5-a2b8-7dca0c8b67bc');
