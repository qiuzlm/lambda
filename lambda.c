#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct term {
    enum { VAR, ABSTR, APP } type;
    union {
        int var;
        struct term* body;
        struct {
            struct term* func;
            struct term* arg;
        } app;
    };
} Term;

void destroy(Term* term) {
    if (term->type == ABSTR) {
        destroy(term->body);
    }
    else if (term->type == APP) {
        destroy(term->app.func);
        destroy(term->app.arg);
    }
    free(term);
}

void print(Term* term) {
    if (term->type == VAR) {
        printf("%d", term->var);
        return;
    }
    if (term->type == ABSTR) {
        printf("λ");
        if (term->body->type != ABSTR) {
            printf(" ");
        }
        print(term->body);
        return;
    }
    if (term->app.func->type == ABSTR) {
        printf("(");
        print(term->app.func);
        printf(")");
    } else {
        print(term->app.func);
    }
    printf(" ");
    if (term->app.arg->type == ABSTR ||
        term->app.arg->type == APP) {
        printf("(");
        print(term->app.arg);
        printf(")");
    } else {
        print(term->app.arg);
    }
}

bool substitute(Term* term, Term* sub, int depth) {
    if (term->type == VAR) {
        if (depth == term->var) {
            *term = *sub;
            return true;
        }
        return false;
    }
    if (term->type == ABSTR) {
        return substitute(term->body, sub, depth + 1);
    }
    if (term->type == APP) {
        return (
            substitute(term->app.func, sub, depth) ||
            substitute(term->app.arg, sub, depth));
    }
}

bool reduce(Term* term) {
    if (term->type != APP) {
        return false;
    }
    reduce(term->app.func);
    if (term->app.func->type == ABSTR) {
        if (substitute(term->app.func, term->app.arg, 0)) {
            *(term->app.func) = *(term->app.func->body);
            *term = *(term->app.func);
        } else {
            *term = *(term->app.func);
            *term = *(term->body);
        }
        reduce(term);
        return true;
    }
    return false;
}

Term* parse(char* string, int start, int end) {
    if (start < 0 || start > end) {
        return NULL;
    }
    if (isspace(string[end])) {
        return parse(string, start, end - 1);
    }
    if (isspace(string[start])) {
        return parse(string, start + 1, end);
    }

    Term* term = (Term*)malloc(sizeof(Term));

    int term_end = end;
    if (string[start] == '\\') {
        int parens = 0;
        while (term_end < end) {
            if (string[end - term_end] == ')') {
                parens--;
                if (parens == 0) {
                    break;
                }
            }
            else if (string[end - term_end] == '(') {
                parens++;
            }
            term_end--;
        }
        term->type = ABSTR;
        term->body = parse(string, start + 1, end);
        return term;
    }
    
    if (string[end] == ')') {
        int parens = 0;
        while (term_end > start) {
            if (string[term_end] == ')') {
                parens++;
            }
            else if (string[term_end] == '(') {
                parens--;
                if (parens == 0) {
                    break;
                }
            }
            term_end--;
        }
        while (term_end > start) {
            if (string[term_end - 1] != ' ' &&
                string[term_end - 1] != '\\')
                break;
            term_end--;
        }
    }
    else {
        while (term_end > start) {
            if (string[term_end] == ' ' ||
                string[term_end] == ')') {
                break;
            }
            term_end--;
        }
    }

    if (term_end == start) {
        if (isdigit(string[start])) {
            term->type = VAR;
            term->var = atoi(string + start);
            return term;
        }
        return parse(string, start + 1, end - 1);
    }

    term->type = APP;
    if (string[start] == '(' && string[end] != ')') {
        term->app.func = parse(string, start, term_end);
        term->app.arg = parse(string, term_end + 1, end);
    } else {
        term->app.func = parse(string, start, term_end - 1);
        term->app.arg = parse(string, term_end, end);
    }
    return term;
}

int main() {
    while (true) {
        printf("λ> ");
        char input[100];
        if (input[0] == 'q') break;
        fgets(input, sizeof(input), stdin);
        Term* term = parse(input, 0, strlen(input) - 1);

        if (term == NULL) {
            printf("Syntax error!\n");
            continue;
        }

        printf("α= ");
        print(term);
        printf("\n");
        reduce(term);
        printf("β= ");
        print(term);
        printf("\n");
        destroy(term);
    }
    return 0;
}
