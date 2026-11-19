#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_WORDS 7
#define MAX_LEN   19
#define MAX_LET   10

typedef struct {
    int       n_words;
    char      words[MAX_WORDS][MAX_LEN];
    char      result[MAX_LEN];
    int       n_letters;
    char      letters[MAX_LET];
    int       idx[26];
    int       leading[MAX_LET];
    long long coef[MAX_LET];
} Puzzle;

static int val[MAX_LET];
static int used[10];
static const Puzzle *P;

static int add_letter(Puzzle *p, char c) {
    int k = c - 'A';
    if (p->idx[k] < 0) {
        if (p->n_letters == MAX_LET) return 0;
        p->idx[k] = p->n_letters;
        p->letters[p->n_letters++] = c;
    }
    return 1;
}

static void add_word(Puzzle *p, const char *w, int sign) {
    int len = (int)strlen(w);
    long long m = 1;
    for (int t = len - 1; t >= 0; t--, m *= 10)
        p->coef[p->idx[w[t] - 'A']] += sign * m;
    p->leading[p->idx[w[0] - 'A']] = 1;
}

static int parse(const char *s, Puzzle *p) {
    memset(p, 0, sizeof *p);
    for (int i = 0; i < 26; i++) p->idx[i] = -1;

    char *cur = p->words[0];
    int len = 0, w = 0, after_eq = 0;

    for (;; s++) {
        char c = *s;
        if (c == ' ') continue;
        if (c >= 'A' && c <= 'Z') {
            if (len >= MAX_LEN - 1 || !add_letter(p, c)) return 0;
            cur[len++] = c;
        } else if (c == '+' || c == '=' || c == '\0') {
            if (len == 0) return 0;
            cur[len] = '\0';
            if (c == '\0') break;
            if (c == '+') {
                if (after_eq || ++w >= MAX_WORDS) return 0;
                cur = p->words[w];
            } else {
                if (after_eq) return 0;
                after_eq = 1;
                p->n_words = w + 1;
                cur = p->result;
            }
            len = 0;
        } else {
            return 0;
        }
    }
    if (!after_eq || p->n_words < 2) return 0;

    for (int i = 0; i < p->n_words; i++) add_word(p, p->words[i], +1);
    add_word(p, p->result, -1);
    return 1;
}

static void print_word(const Puzzle *p, const char *w) {
    for (; *w; w++) putchar('0' + val[p->idx[*w - 'A']]);
}

static void print_solution(const Puzzle *p) {
    for (int i = 0; i < p->n_words; i++) {
        if (i) printf(" + ");
        print_word(p, p->words[i]);
    }
    printf(" = ");
    print_word(p, p->result);
    putchar('\n');
}

static void reset(void) {
    for (int i = 0; i < MAX_LET; i++) val[i] = -1;
    for (int d = 0; d < 10; d++) used[d] = 0;
}

/* v0: БАЗОВОЕ РЕШЕНИЕ */
static long long word_value(const char *w) {
    long long v = 0;
    for (; *w; w++) v = v * 10 + val[P->idx[*w - 'A']];
    return v;
}

static int check_v0(void) {
    long long sum = 0;
    for (int i = 0; i < P->n_words; i++) {
        if (val[P->idx[P->words[i][0] - 'A']] == 0) return 0;
        sum += word_value(P->words[i]);
    }
    if (val[P->idx[P->result[0] - 'A']] == 0) return 0;
    return sum == word_value(P->result);
}

static int rec_v0(int k) {
    if (k == P->n_letters) return check_v0();
    for (int d = 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[k] = d;
        if (rec_v0(k + 1)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_v0(const Puzzle *p) {
    P = p; reset();
    return rec_v0(0);
}

static int rec_v1(int k) {
    if (k == P->n_letters) {
        long long s = 0;
        for (int i = 0; i < P->n_letters; i++) {
            if (P->leading[i] && val[i] == 0) return 0;
            s += P->coef[i] * val[i];
        }
        return s == 0;
    }
    for (int d = 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[k] = d;
        if (rec_v1(k + 1)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_v1(const Puzzle *p) {
    P = p; reset();
    return rec_v1(0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s \"SEND + MORE = MONEY\"\n", argv[0]);
        return 1;
    }
    Puzzle p;
    if (!parse(argv[1], &p)) { fprintf(stderr, "bad puzzle\n"); return 1; }
    
    clock_t t0 = clock();
    int res = solve_v1(&p);
    double t = (double)(clock() - t0) / CLOCKS_PER_SEC;
    
    if (res) print_solution(&p);
    else puts("no solution");
    printf("Time: %.4f sec.\n", t);
    return 0;
}