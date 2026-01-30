#include "DancingLinksDS.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>

#define N 9
#define NUM_COLS 324
#define NUM_ROWS 729

static inline int encode(int r, int c, int d) { return r*81 + c*9 + d; }
static inline void decode(int id, int *r, int *c, int *d) { *d = id%9; *c = (id/9)%9; *r = id/81; }

static inline void get_cols(int r, int c, int d, int *out) {
    int box = (r/3)*3 + c/3;
    out[0] = r*9 + c;
    out[1] = 81 + r*9 + d;
    out[2] = 162 + c*9 + d;
    out[3] = 243 + box*9 + d;
}

static void link_row(DLX *dlx, int r, int c, int d) {
    int id = encode(r, c, d);
    int base = id * 4;
    int cols[4];
    get_cols(r, c, d, cols);

    for (int i = 0; i < 4; i++) {
        Node *node = &dlx->all_nodes[base + i];
        ColumnHeader *col = dlx->columns[cols[i]];
        node->row_id = id;
        node->column = col;
        node->up = col->node.up;
        node->down = &col->node;
        col->node.up->down = node;
        col->node.up = node;
        col->size++;
    }

    for (int i = 0; i < 4; i++) {
        Node *node = &dlx->all_nodes[base + i];
        node->left = &dlx->all_nodes[base + (i+3)%4];
        node->right = &dlx->all_nodes[base + (i+1)%4];
    }
}

DLX *dlx_create(void) {
    DLX *dlx = malloc(sizeof(DLX));
    dlx->solution = malloc(81 * sizeof(int));
    dlx->all_nodes = malloc(NUM_ROWS * 4 * sizeof(Node));
    dlx->root = malloc(sizeof(ColumnHeader));
    dlx->columns = malloc(NUM_COLS * sizeof(ColumnHeader *));
    dlx->root->node.column = dlx->root;
    for (int i = 0; i < NUM_COLS; i++) {
        dlx->columns[i] = malloc(sizeof(ColumnHeader));
        dlx->columns[i]->node.column = dlx->columns[i];
    }
    dlx_reset(dlx);
    return dlx;
}

void dlx_reset(DLX *dlx) {
    dlx->root->node.left = dlx->root->node.right = &dlx->root->node;
    for (int i = 0; i < NUM_COLS; i++) {
        ColumnHeader *col = dlx->columns[i];
        col->node.up = col->node.down = &col->node;
        col->size = 0;
        col->node.left = dlx->root->node.left;
        col->node.right = &dlx->root->node;
        dlx->root->node.left->right = &col->node;
        dlx->root->node.left = &col->node;
    }
    for (int r = 0; r < N; r++)
        for (int c = 0; c < N; c++)
            for (int d = 0; d < N; d++)
                link_row(dlx, r, c, d);
    dlx->solutions_found = 0;
}

void dlx_destroy(DLX *dlx) {
    for (int i = 0; i < NUM_COLS; i++) free(dlx->columns[i]);
    free(dlx->columns);
    free(dlx->all_nodes);
    free(dlx->root);
    free(dlx->solution);
    free(dlx);
}

static void cover(ColumnHeader *col) {
    col->node.right->left = col->node.left;
    col->node.left->right = col->node.right;
    for (Node *r = col->node.down; r != &col->node; r = r->down)
        for (Node *n = r->right; n != r; n = n->right) {
            n->down->up = n->up;
            n->up->down = n->down;
            n->column->size--;
        }
}

static void uncover(ColumnHeader *col) {
    for (Node *r = col->node.up; r != &col->node; r = r->up)
        for (Node *n = r->left; n != r; n = n->left) {
            n->column->size++;
            n->down->up = n;
            n->up->down = n;
        }
    col->node.right->left = &col->node;
    col->node.left->right = &col->node;
}

static ColumnHeader *choose_col(DLX *dlx) {
    ColumnHeader *best = NULL;
    int min = INT_MAX;
    for (Node *n = dlx->root->node.right; n != &dlx->root->node; n = n->right) {
        ColumnHeader *c = (ColumnHeader *)n;
        if (c->size < min) { min = c->size; best = c; }
    }
    return best;
}

static void apply_clue(DLX *dlx, int r, int c, int d) {
    int cols[4];
    get_cols(r, c, d, cols);
    for (int i = 0; i < 4; i++) cover(dlx->columns[cols[i]]);
}

static bool search(DLX *dlx, int depth) {
    if (dlx->root->node.right == &dlx->root->node) { dlx->solutions_found++; return true; }
    ColumnHeader *col = choose_col(dlx);
    if (col->size == 0) return false;

    int n = col->size;
    Node **rows = malloc(n * sizeof(Node *));
    int i = 0;
    for (Node *r = col->node.down; r != &col->node; r = r->down) rows[i++] = r;
    for (int i = n-1; i > 0; i--) { int j = rand()%(i+1); Node *t = rows[i]; rows[i] = rows[j]; rows[j] = t; }

    cover(col);
    for (int i = 0; i < n; i++) {
        Node *row = rows[i];
        dlx->solution[depth] = row->row_id;
        for (Node *r = row->right; r != row; r = r->right) cover(r->column);
        if (search(dlx, depth + 1)) { free(rows); uncover(col); return true; }
        for (Node *r = row->left; r != row; r = r->left) uncover(r->column);
    }
    uncover(col);
    free(rows);
    return false;
}

static int count(DLX *dlx, int depth, int max) {
    if (dlx->root->node.right == &dlx->root->node) return ++dlx->solutions_found;
    ColumnHeader *col = choose_col(dlx);
    if (col->size == 0) return dlx->solutions_found;
    cover(col);
    for (Node *row = col->node.down; row != &col->node; row = row->down) {
        dlx->solution[depth] = row->row_id;
        for (Node *r = row->right; r != row; r = r->right) cover(r->column);
        count(dlx, depth + 1, max);
        if (dlx->solutions_found >= max) {
            for (Node *r = row->left; r != row; r = r->left) uncover(r->column);
            uncover(col);
            return dlx->solutions_found;
        }
        for (Node *r = row->left; r != row; r = r->left) uncover(r->column);
    }
    uncover(col);
    return dlx->solutions_found;
}

static void extract(DLX *dlx, int grid[9][9]) {
    for (int i = 0; i < 81; i++) { int r,c,d; decode(dlx->solution[i], &r, &c, &d); grid[r][c] = d+1; }
}

bool sudoku_generate(int grid[9][9]) {
    DLX *dlx = dlx_create();
    bool ok = search(dlx, 0);
    if (ok) extract(dlx, grid);
    dlx_destroy(dlx);
    return ok;
}

int sudoku_create_puzzle(int full[9][9], int puzzle[9][9]) {
    for (int i = 0; i < 81; i++) puzzle[i/9][i%9] = full[i/9][i%9];

    int pos[81]; for (int i = 0; i < 81; i++) pos[i] = i;
    for (int i = 80; i > 0; i--) { int j = rand()%(i+1); int t = pos[i]; pos[i] = pos[j]; pos[j] = t; }

    DLX *dlx = dlx_create();
    int clues = 81;

    for (int i = 0; i < 81; i++) {
        int r = pos[i]/9, c = pos[i]%9;
        if (!puzzle[r][c]) continue;
        int saved = puzzle[r][c];
        puzzle[r][c] = 0;

        dlx_reset(dlx);
        for (int rr = 0; rr < 9; rr++)
            for (int cc = 0; cc < 9; cc++)
                if (puzzle[rr][cc]) apply_clue(dlx, rr, cc, puzzle[rr][cc]-1);

        dlx->solutions_found = 0;
        if (count(dlx, 0, 2) >= 2) puzzle[r][c] = saved;
        else clues--;
    }

    dlx_destroy(dlx);
    return clues;
}

static void pretty(int g[9][9]) {
    for (int r = 0; r < 9; r++) {
        if (r && r%3 == 0) printf("------+-------+------\n");
        for (int c = 0; c < 9; c++) {
            if (c && c%3 == 0) printf("| ");
            printf("%c ", g[r][c] ? '0'+g[r][c] : '.');
        }
        printf("\n");
    }
}

void line(int grid[9][9]) {
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            putchar(grid[r][c] ? '0' + grid[r][c] : '.');
    putchar('\n');
}

int main(void) {
    srand(time(NULL));
    int full[9][9], puzzle[9][9];

    sudoku_generate(full);
    int clues = sudoku_create_puzzle(full, puzzle);

    printf("Full:\n");
    pretty(full);
    printf("\nPuzzle (%d clues):\n", clues);
    pretty(puzzle);
    line(puzzle);
}
