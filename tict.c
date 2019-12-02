#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>
#include "tict.h"

#define LONGEST_WORD 48
#define UNUSED(x) (void)x

const char *argp_program_bug_address = "cheon0112358d@gmail.com";
const char *argp_program_version = "version 1.0.0";
const char *argp_doc = "dict in terminal which saves query words in local database";

static int parse_opt(int key, char *arg, struct argp_state *state) {
    sqlite3 *words_db = state->input;
    switch(key) {
        case 'c':
            clean_db(words_db);
            printf("delete all data in database\n");
            break;
        case 'q': {
            Word *word = query_word(words_db, arg);
            if(word!=NULL) {
                print_word(word);
                free_word(word);
            }
            break;
        }
        case 'r': {
            Word *word = random_word(words_db);
            if(word==NULL)
                printf("there is no word in database\n");
            else {
                print_word(word);
                free_word(word);
            }
            break;
        }
        case 't': {
            Word ** result = (Word**)malloc(sizeof(Word*)*10);
            for(int i=0; i<10; i++) {
                result[i] = (Word*)malloc(sizeof(Word));
                result[i]->word = (char *)calloc(sizeof(char), LONGEST_WORD);
                result[i]->pronunciation = (char *)calloc(sizeof(char), LONGEST_WORD);
                result[i]->meaning = (char *)calloc(sizeof(char), BUFSIZ);
            }
            result = top_word(words_db, result);
            if(strcmp(result[0]->word, "")==0)
                printf("there is no word in database\n");
            else {
                for(int i=0; i<10; i++) {
                    printf("========================================\n");
                    print_word(result[i]);
                    free_word(result[i]);
                }
            }
            free(result);
            break;
        }
        case 'u':
            printf("updating...\n");
            update_db(words_db);
            printf("update success, you can clear $HOME/.words now.\n");
            break;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // init
    char *path = (char *)calloc(sizeof(char), LONGEST_WORD);
    snprintf(path, LONGEST_WORD, "%s/.config/tict/words.db", getenv("HOME"));
    sqlite3 *words_db = NULL;
    int result = access(path, W_OK);
    sqlite3_open(path, &words_db);
    if(result!=0) {
        char *err;
        if(sqlite3_exec(words_db, "create table words(id integer primary key autoincrement, word text, pronunciation text, meaning text, query_count integer default 0);", NULL, NULL, &err)!=SQLITE_OK)
        perror(err);
    }

    struct argp_option options[] =
        {
         {"clean", 'c', 0, 0, "delete data in database", 0},
         {"query", 'q', "WORD", 0, "query a word from datase", 0},
         {"random", 'r', 0, 0, "show the top 10 words you most queried", 0},
         {"top", 't', 0, 0, "show the top 10 words you most queried", 0},
         {"update", 'u', 0, 0, "insert cache data to database", 0},
         {0}
        };

    struct argp argp = {options, parse_opt, "[WORD]", argp_doc, NULL, NULL, NULL};
    argp_parse(&argp, argc, argv, 0, 0, words_db);

    // clean
    free(path);
    sqlite3_close(words_db);
    return 0;
}

void print_word(Word *word) {
    printf("%s\e[32;1mUK\e[0m: %s%s", word->word,word->pronunciation,word->meaning);
}

void free_word(Word *word) {
    free(word->word);
    free(word->meaning);
    free(word->pronunciation);
    free(word);
}

int query_callback(void *data, int column_count, char **column_value, char **column_name) {
    UNUSED(column_count);
    UNUSED(column_name);
    Word *word = (Word *)data;
    word->word = (char *)calloc(sizeof(char), LONGEST_WORD);
    word->pronunciation = (char *)calloc(sizeof(char), LONGEST_WORD);
    word->meaning = (char *)calloc(sizeof(char), BUFSIZ);
    strncpy(word->word, column_value[1], strlen(column_value[1]));
    strncpy(word->pronunciation, column_value[2], strlen(column_value[2]));
    strncpy(word->meaning, column_value[3], strlen(column_value[3]));
	return 0;
}

void update_db(sqlite3 *words_db) {
    char *path = (char *)calloc(sizeof(char), LONGEST_WORD);
    snprintf(path, LONGEST_WORD, "%s/.words", getenv("HOME"));
    FILE *words = fopen(path, "r");
    if(words==NULL)
        perror(path);
    Word *word = (Word *)malloc(sizeof(Word));
    word->word = (char *)calloc(sizeof(char), LONGEST_WORD);
    word->pronunciation = (char *)calloc(sizeof(char), LONGEST_WORD);
    word->meaning = (char *)calloc(sizeof(char), BUFSIZ);

    char line[BUFSIZ];
    int line_count = 1;

    while((fgets(line, BUFSIZ, words)) != NULL) {
        if(strcmp(line, "\n")==0) {
            line_count = 1;
            sprintf(line,
                    "insert into words(word, pronunciation, meaning) values(\"%s\", \"%s\", \"%s\");",
                    word->word, word->pronunciation, word->meaning);
            sqlite3_exec(words_db, line, NULL, NULL, NULL);
            memset(word->word, 0, LONGEST_WORD);
            memset(word->pronunciation, 0, LONGEST_WORD);
            memset(word->meaning, 0, BUFSIZ);
            continue;
        }
        switch(line_count) {
        case 1:
            strncpy(word->word, line, strlen(line));
            break;
        case 2:
            if(line[0]=='[')
                strncpy(word->pronunciation, line, strlen(line));
            break;
        default:
            if(line[0]!='-')
                strncat(word->meaning, line, strlen(line));
            break;
        }
        line_count++;
    }
    free_word(word);
    fclose(words);
    free(path);
}

void clean_db(sqlite3 *words_db) {
    sqlite3_exec(words_db, "delete from words; vacuum;", NULL, NULL, NULL);
    sqlite3_exec(words_db, "UPDATE \"main\".\"sqlite_sequence\" SET seq = 0 WHERE name = 'words';", NULL, NULL, NULL);
}

Word * query_word(sqlite3 *words_db, char *word) {
    Word *result = (Word *)malloc(sizeof(Word));
    result->word = NULL;
    char *sql = (char *)calloc(sizeof(char) , BUFSIZ);
    sprintf(sql, "select * from words where word=\"%s\n\"", word);
    sqlite3_exec(words_db, sql, query_callback, result, NULL);
    if(result->word == NULL) {
        free(result);
        result = NULL;
    }
    free(sql);
    return result;
}

Word ** top_word(sqlite3 *words_db, Word **result) {
    sqlite3_stmt *stmt = NULL;
    const char *zTail;

    if(sqlite3_prepare(words_db, "select * from words order by query_count desc limit 10;", -1, &stmt, &zTail) == SQLITE_OK) {
        int i = 0;
        while(sqlite3_step(stmt) == SQLITE_ROW) {
            strcpy(result[i]->word, (char *)sqlite3_column_text(stmt, 1));
            strcpy(result[i]->pronunciation, (char *)sqlite3_column_text(stmt, 2));
            strcpy(result[i]->meaning, (char *)sqlite3_column_text(stmt, 3));
            i++;
        }
    }
    return result;
}

Word * random_word(sqlite3 *words_db) {
    Word *result = (Word *)malloc(sizeof(Word));
    result->word = NULL;
    sqlite3_exec(words_db, "select * from words order by random() limit 1;", query_callback, result, NULL);
    if(result->word == NULL) {
        free(result);
        result = NULL;
    }
    return result;
}
