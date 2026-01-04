#ifndef LOAD_TEST_DATA_H
#define LOAD_TEST_DATA_H

/* 在程序启动时执行，向 users 表中插入测试学生数据（10 条） */
void load_test_user_data(void);

/* 在程序启动时执行，向 questions 表中插入测试题目数据（至少 5 道） */
void load_sample_questions(void);

#endif
