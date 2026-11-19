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

static int       order[MAX_LET];
static long long neg_sum[MAX_LET + 1];
static long long pos_sum[MAX_LET + 1];

static int rec_v2(int k, long long partial) {
    int n = P->n_letters;
    if (k == n) return partial == 0;
    if (partial + 9 * neg_sum[k] > 0 || partial + 9 * pos_sum[k] < 0) return 0;

    int li = order[k];
    for (int d = P->leading[li] ? 1 : 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[li] = d;
        if (rec_v2(k + 1, partial + P->coef[li] * d)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int solve_v2(const Puzzle *p) {
    P = p; reset();
    int n = p->n_letters;
    for (int i = 0; i < n; i++) order[i] = i;
    for (int i = 1; i < n; i++) {
        int x = order[i], j = i - 1;
        while (j >= 0 && llabs(p->coef[order[j]]) < llabs(p->coef[x])) {
            order[j + 1] = order[j]; j--;
        }
        order[j + 1] = x;
    }
    neg_sum[n] = pos_sum[n] = 0;
    for (int k = n - 1; k >= 0; k--) {
        long long c = p->coef[order[k]];
        neg_sum[k] = neg_sum[k + 1] + (c < 0 ? c : 0);
        pos_sum[k] = pos_sum[k + 1] + (c > 0 ? c : 0);
    }
    return rec_v2(0, 0);
}

static int ncols;
static int col_cnt[MAX_LEN];
static int col_let[MAX_LEN][MAX_WORDS];
static int res_let[MAX_LEN];

static int col_step(int i, int carry);

static int fill(int i, const int *todo, int nt, int k, int carry) {
    if (k == nt) {
        int s = carry;
        for (int j = 0; j < col_cnt[i]; j++) s += val[col_let[i][j]];
        if (s % 10 != val[res_let[i]]) return 0;
        return col_step(i + 1, s / 10);
    }
    int l = todo[k];
    for (int d = P->leading[l] ? 1 : 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[l] = d;
        if (fill(i, todo, nt, k + 1, carry)) return 1;
        used[d] = 0; val[l] = -1;
    }
    return 0;
}

static int col_step(int i, int carry) {
    if (i == ncols) return carry == 0;
    int todo[MAX_WORDS + 1], nt = 0;
    for (int j = 0; j <= col_cnt[i]; j++) {
        int l = (j < col_cnt[i]) ? col_let[i][j] : res_let[i];
        if (val[l] >= 0) continue;
        int dup = 0;
        for (int t = 0; t < nt; t++) if (todo[t] == l) dup = 1;
        if (!dup) todo[nt++] = l;
    }
    return fill(i, todo, nt, 0, carry);
}

static int solve_v3(const Puzzle *p) {
    P = p; reset();
    ncols = (int)strlen(p->result);
    memset(col_cnt, 0, sizeof col_cnt);
    for (int i = 0; i < p->n_words; i++) {
        int len = (int)strlen(p->words[i]);
        if (len > ncols) return 0;
        for (int pos = 0; pos < len; pos++) {
            char c = p->words[i][len - 1 - pos];
            col_let[pos][col_cnt[pos]++] = p->idx[c - 'A'];
        }
    }
    for (int pos = 0; pos < ncols; pos++)
        res_let[pos] = p->idx[p->result[ncols - 1 - pos] - 'A'];
    return col_step(0, 0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s \"SEND + MORE = MONEY\"\n", argv[0]);
        return 1;
    }
    Puzzle p;
    if (!parse(argv[1], &p)) { fprintf(stderr, "bad puzzle\n"); return 1; }
    
    clock_t t0 = clock();
    int res = solve_v3(&p);
    double t = (double)(clock() - t0) / CLOCKS_PER_SEC;
    
    if (res) print_solution(&p);
    else puts("no solution");
    printf("Time: %.4f sec.\n", t);
    return 0;
}