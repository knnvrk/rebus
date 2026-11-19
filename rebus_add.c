#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TERMS 8
#define MAX_LEN   10
#define MAX_LET   10

typedef struct {
    int  n;
    char ops[MAX_TERMS];
    char w[MAX_TERMS][MAX_LEN];
} Expr;

typedef struct {
    Expr L, R;
    int  n_letters;
    char letters[MAX_LET];
    int  idx[26];
    int  leading[MAX_LET];
    int  minpos[MAX_LET];
    int  order[MAX_LET];
    int  check_k[MAX_LET];
    int  has_div;
} Puzzle;

static int val[MAX_LET];
static int used[10];
static const Puzzle *P;
static long long pow10_[12];

static int is_op(char c) { return c == '+' || c == '-' || c == '*' || c == '/'; }

static int parse_expr(const char *s, Expr *e) {
    e->n = 0;
    int len = 0;
    char op = '+';
    for (;; s++) {
        char c = *s;
        if (c >= 'A' && c <= 'Z') {
            if (len >= MAX_LEN - 1) return 0;
            e->w[e->n][len++] = c;
        } else if (is_op(c) || c == '\0') {
            if (len == 0) return 0;
            e->w[e->n][len] = '\0';
            e->ops[e->n++] = op;
            if (c == '\0') break;
            if (e->n >= MAX_TERMS) return 0;
            op = c; len = 0;
        } else {
            return 0;
        }
    }
    return 1;
}

static int size_ok(const Expr *e) {
    int chain = 0;
    for (int i = 0; i < e->n; i++) {
        if (i == 0 || e->ops[i] == '+' || e->ops[i] == '-') chain = 0;
        chain += (int)strlen(e->w[i]);
        if (chain > 18) return 0;
    }
    return 1;
}

static int add_letter(Puzzle *p, char c) {
    int k = c - 'A';
    if (p->idx[k] < 0) {
        if (p->n_letters == MAX_LET) return 0;
        p->idx[k] = p->n_letters;
        p->minpos[p->n_letters] = 1000;
        p->letters[p->n_letters++] = c;
    }
    return 1;
}

static int register_expr(Puzzle *p, const Expr *e) {
    for (int i = 0; i < e->n; i++) {
        int len = (int)strlen(e->w[i]);
        for (int t = 0; t < len; t++) {
            if (!add_letter(p, e->w[i][t])) return 0;
            int li = p->idx[e->w[i][t] - 'A'];
            int pos = len - 1 - t;
            if (pos < p->minpos[li]) p->minpos[li] = pos;
        }
        p->leading[p->idx[e->w[i][0] - 'A']] = 1;
    }
    return 1;
}

static void rewrite_division(Puzzle *p) {
    if (p->L.n == 1 && p->R.n == 2 && p->R.ops[1] == '/') {   /* C = A / B */
        Expr t = p->L; p->L = p->R; p->R = t;
    }
    if (p->L.n == 2 && p->L.ops[1] == '/' && p->R.n == 1) {
        Expr nl, nr;
        nl.n = 2;
        nl.ops[0] = '+'; strcpy(nl.w[0], p->L.w[1]);   /* B */
        nl.ops[1] = '*'; strcpy(nl.w[1], p->R.w[0]);   /* C */
        nr.n = 1;
        nr.ops[0] = '+'; strcpy(nr.w[0], p->L.w[0]);   /* A */
        p->L = nl; p->R = nr;
    }
}

static int has_division(const Expr *e) {
    for (int i = 1; i < e->n; i++) if (e->ops[i] == '/') return 1;
    return 0;
}

static int parse(const char *src, Puzzle *p) {
    char buf[256], *eq;
    size_t m = 0;
    memset(p, 0, sizeof *p);
    for (int i = 0; i < 26; i++) p->idx[i] = -1;
    for (const char *c = src; *c; c++) {
        if (*c == ' ') continue;
        if (m + 1 >= sizeof buf) return 0;
        buf[m++] = *c;
    }
    buf[m] = '\0';
    eq = strchr(buf, '=');
    if (!eq || strchr(eq + 1, '=')) return 0;
    *eq = '\0';
    if (!parse_expr(buf, &p->L) || !parse_expr(eq + 1, &p->R)) return 0;
    if (!size_ok(&p->L) || !size_ok(&p->R)) return 0;

    rewrite_division(p);
    p->has_div = has_division(&p->L) || has_division(&p->R);

    if (!register_expr(p, &p->L) || !register_expr(p, &p->R)) return 0;

    int n = p->n_letters;
    for (int i = 0; i < n; i++) p->order[i] = i;
    for (int i = 1; i < n; i++) {
        int x = p->order[i], j = i - 1;
        while (j >= 0 && p->minpos[p->order[j]] > p->minpos[x]) {
            p->order[j + 1] = p->order[j];
            j--;
        }
        p->order[j + 1] = x;
    }

    for (int j = 0; j < n; j++) {
        p->check_k[j] = 0;
        if (!p->has_div && j + 1 < n &&
            p->minpos[p->order[j + 1]] > p->minpos[p->order[j]])
            p->check_k[j] = p->minpos[p->order[j + 1]];
    }
    return 1;
}

static long long word_val(const char *w, int k) {
    int len = (int)strlen(w);
    int start = (k > 0 && len > k) ? len - k : 0;
    long long v = 0;
    for (int t = start; t < len; t++) v = v * 10 + val[P->idx[w[t] - 'A']];
    return v;
}

static int eval_expr(const Expr *e, int k, long long *out) {
    long long mod = k ? pow10_[k] : 0;
    long long total = 0, term = 0;
    int sign = 1;
    for (int i = 0; i < e->n; i++) {
        long long v = word_val(e->w[i], k);
        char op = e->ops[i];
        if (i == 0) {
            term = v;
        } else if (op == '+' || op == '-') {
            total += sign * term;
            if (mod) total %= mod;
            sign = (op == '+') ? 1 : -1;
            term = v;
        } else if (op == '*') {
            term *= v;
            if (mod) term %= mod;
        } else {
            if (v == 0 || term % v != 0) return 0;
            term /= v;
        }
    }
    total += sign * term;
    if (mod) { total %= mod; if (total < 0) total += mod; }
    *out = total;
    return 1;
}

static int equal_mod(int k) {
    long long a, b;
    if (!eval_expr(&P->L, k, &a) || !eval_expr(&P->R, k, &b)) return 0;
    return a == b;
}

static void print_solution_from_source(const char *src) {
    for (const char *c = src; *c; c++) {
        if (*c >= 'A' && *c <= 'Z') putchar('0' + val[P->idx[*c - 'A']]);
        else putchar(*c);
    }
    putchar('\n');
}

static void reset(void) {
    for (int i = 0; i < MAX_LET; i++) val[i] = -1;
    for (int d = 0; d < 10; d++) used[d] = 0;
    pow10_[0] = 1;
    for (int i = 1; i < 12; i++) pow10_[i] = pow10_[i - 1] * 10;
}

static int rec_v0(int j) {
    if (j == P->n_letters) {
        for (int i = 0; i < P->n_letters; i++)
            if (P->leading[i] && val[i] == 0) return 0;
        return equal_mod(0);
    }
    for (int d = 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[j] = d;
        if (rec_v0(j + 1)) return 1;
        used[d] = 0;
    }
    return 0;
}

static int rec_v1(int j) {
    int n = P->n_letters;
    if (j == n) return equal_mod(0);
    int li = P->order[j];
    for (int d = P->leading[li] ? 1 : 0; d < 10; d++) {
        if (used[d]) continue;
        used[d] = 1; val[li] = d;
        int ok = 1;
        if (P->check_k[j] > 0) ok = equal_mod(P->check_k[j]);
        if (ok && rec_v1(j + 1)) return 1;
        used[d] = 0; val[li] = -1;
    }
    return 0;
}

static int solve(const Puzzle *p, int version) {
    P = p; reset();
    return version == 0 ? rec_v0(0) : rec_v1(0);
}

static double bench_one(const Puzzle *p, int version) {
    clock_t t0 = clock();
    int runs = 0;
    do { solve(p, version); runs++; }
    while ((double)(clock() - t0) / CLOCKS_PER_SEC < 0.2);
    return (double)(clock() - t0) / CLOCKS_PER_SEC / runs * 1000.0;
}

static int run_bench(void) {
    const char *tests[] = {
        "AB * C = DEF",
        "ABC / D = EF",
        "MONEY - SEND = MORE",
        "A + B * C = DE",
        "SEND + MORE = MONEY",
        "ABC * DE = FGHIJ"
    };
    printf("%-26s%12s%12s\n", "puzzle (ms per run)", "v0", "v1");
    for (int t = 0; t < (int)(sizeof tests / sizeof tests[0]); t++) {
        Puzzle p;
        if (!parse(tests[t], &p)) { printf("bad: %s\n", tests[t]); continue; }
        printf("%-26s%12.4f%12.4f\n", tests[t], bench_one(&p, 0), bench_one(&p, 1));
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--bench") == 0) return run_bench();
    if (argc < 2) {
        fprintf(stderr, "usage: %s \"AB * C = DEF\" [0|1]\n       %s --bench\n",
                argv[0], argv[0]);
        return 1;
    }
    int version = (argc >= 3) ? atoi(argv[2]) : 1;
    Puzzle p;
    if (!parse(argv[1], &p)) { fprintf(stderr, "bad puzzle\n"); return 1; }
    if (solve(&p, version)) print_solution_from_source(argv[1]);
    else                    puts("no solution");
    return 0;
}