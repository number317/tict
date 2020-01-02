#ifndef _TICT_H
#define _TICT_H

#include <stdio.h>
#include "sqlite3.h"
typedef struct Word {
    char *word;
    char *pronunciation;
    char *meaning;
    int query_count;
} Word;

void usage();
void print_word(Word *word);
void free_word(Word *word);
int query_callback(void *data, int column_count, char **column_value, char **column_name);
void update_db(sqlite3 *words_db);
void clean_db(sqlite3 *words_db);
Word * query_word(sqlite3 *words_db, char *word);
Word ** top_word(sqlite3 *words_db, Word **result);
Word * random_word(sqlite3 *words_db);
void dump_word(sqlite3 *words_db);

#endif
