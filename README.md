# VocabularyScale
GDUT 计算机学院 2025级 程序设计课设 ( 题目1: 英语练习系统 ) personal repository

# 食用方法

编译

```shell
gcc -g *.c lib\sqlite3.c -o VocabularScale.exe
```

运行 `VocabularScale.exe` 即可


# 程序结构

- `lib` 程序所依赖的外部库。
- `app.c` 主程序，包含 `main()`，负责 CLI 输出。
- `database.c` 包含数据库操作。
- `load_test_data.c` 包含数据库初始化操作，便于管理员测试数据。
- `file_io.c` 负责将 `.db` 中的数据写入 `.txt` 文件
- `question_list.c` 


# 测试数据说明

本程序附带测试数据。由 `load_test_data.c` 实现。

测试用户如下：
| 用户名 | 用户类型(级别) | 班级 | 所属教师 |
| --- | --- | --- | --- |
| stu0 | 学生(2) | 1 | teacher0 |
| stu1 | 学生(2) | 1 | teacher0 |
| stu2 | 学生(2) | 1 | teacher0 |
| stu3 | 学生(2) | 1 | teacher0 |
| stu4 | 学生(2) | 1 | teacher0 |
| stu5 | 学生(2) | 2 | teacher1 |
| stu6 | 学生(2) | 2 | teacher1 |
| stu7 | 学生(2) | 2 | teacher1 |
| stu8 | 学生(2) | 2 | teacher1 |
| stu9 | 学生(2) | 2 | teacher1 |
| teacher0 | 教师(1) | 1 | NULL |
| teacher1 | 教师(1) | 2 | NULL |

测试用户的 UUID 均为动态生成。


# 更新日志

## 2026/01/05

各项功能测试均正常，算是一个完成体了。


# 参考

[SQLite Document](https://sqlite.org/index.html)  

