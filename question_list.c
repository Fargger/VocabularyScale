#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "question_list.h"
#include "lib/sqlite3.h"

struct QuestionNode* getQuestionsLL(int* count) {
    sqlite3 *db;
    if (sqlite3_open("vocab_system.db", &db) != SQLITE_OK) {
        if (count) *count = 0;
        return NULL;
    }

    const char* sql = "SELECT qid, word, translate FROM questions ORDER BY qid ASC";
    sqlite3_stmt* stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        if (count) *count = 0;
        return NULL;
    }

    struct QuestionNode* head = NULL;
    struct QuestionNode* tail = NULL;
    int cnt = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        struct QuestionNode* node = (struct QuestionNode*)malloc(sizeof(struct QuestionNode));
        if (!node) break;
        memset(node, 0, sizeof(*node));
        node->q.qid = sqlite3_column_int(stmt, 0);
        const unsigned char* w = sqlite3_column_text(stmt, 1);
        const unsigned char* t = sqlite3_column_text(stmt, 2);
        if (w) strncpy(node->q.word, (const char*)w, MAX_WORD_LENGTH - 1);
        if (t) strncpy(node->q.translate, (const char*)t, MAX_TRANS_LENGTH - 1);
        node->next = NULL;

        if (!head) head = tail = node;
        else { tail->next = node; tail = node; }
        cnt++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (count) *count = cnt;
    return head;
}

void freeQuestionList(struct QuestionNode* head) {
    struct QuestionNode* cur = head;
    while (cur) {
        struct QuestionNode* tmp = cur->next;
        free(cur);
        cur = tmp;
    }
}
