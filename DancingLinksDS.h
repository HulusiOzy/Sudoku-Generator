#ifndef DLX_H
#define DLX_H

#include <stdbool.h>

typedef struct Node Node;
typedef struct ColumnHeader ColumnHeader;

struct Node {
    Node *left, *right, *up, *down;
    ColumnHeader *column;
    int row_id;
};

struct ColumnHeader {
    Node node;
    int size;
};

typedef struct {
    ColumnHeader *root;
    ColumnHeader **columns;
    Node *all_nodes;
    int *solution;
    int solutions_found;
} DLX;

DLX *dlx_create(void);
void dlx_reset(DLX *dlx);
void dlx_destroy(DLX *dlx);
bool sudoku_generate(int grid[9][9]);
int sudoku_create_puzzle(int full[9][9], int puzzle[9][9]);

#endif
