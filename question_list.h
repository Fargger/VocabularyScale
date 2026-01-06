#ifndef QUESTION_LIST_H
#define QUESTION_LIST_H

#include "database.h"

/* 链表节点，保存一条题目信息 */
struct QuestionNode {
    struct Question q;
    struct QuestionNode* next;
};

/*
 * 获取题目链表（按 qid 升序）
 * 返回链表头，count 输出题目数量（可为 NULL）
 */
struct QuestionNode* getQuestionsLL(int* count);

/* 释放题目链表 */
void freeQuestionList(struct QuestionNode* head);

#endif /* QUESTION_LIST_H */
