# VocabularyScale
GDUT 计算机学院 2025级 程序设计课设 ( 题目1: 英语练习系统 ) personal repository

# 程序结构

- `lib` 程序所依赖的外部库。
- `app.c` 主程序，包含 `main()`，负责 CLI 输出。
- `database.c` 包含数据库操作。
- `load_test_data.c` 包含数据库初始化操作，便于管理员测试数据。
- `file_io.c` 负责将 `.db` 中的数据写入 `.txt` 文件
- `question_list.c` 

# 食用方法

编译

```shell
gcc -g *.c lib\sqlite3.c -o VocabularScale.exe
```

运行 `VocabularScale.exe` 即可