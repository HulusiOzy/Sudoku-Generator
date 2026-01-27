#ifndef SUDOKU_DLX_H
#define SUDOKU_DLX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define N 9
#define BOX_SIZE 3
#define NUM_CONSTRAINTS 324 //4x81
#define MAX_ROWS 729        //9x9x9

typedef struct Node Node;
typedef struct ColumnHeader ColumnHeader;

struct Node {   //No size here, only col headers have size
    Node *left, *right, *up, *down; //Four way links
    ColumnHeader *column;           //C
    int row_id;
};

struct ColumnHeader {
    Node node;      //Links
    int size;       //Nodes in this col
    char name[16];  //For debugging
};

typedef struct {
    ColumnHeader *root;     //Master Header
    ColumnHeader **columns; //Keep the 324 column pointers
    Node *all_nodes;        //Flat arr of 729x4 nodes
    int *solution;          //Stack of chosen row ids
    int solution_size;      //Curr depth in the search tree
    int solutions_found;    //Total count of solutions found
    int max_solution_size;  //Cap of the solution arr
} DLX;

// Meant to return unique ID for constraint
static inline int encode_row_id(int row, int col, int digit) { 
    return row * 81 + col * 9 + digit;
}

// Decode row_id back to row, col, digit for printing solutions
static inline void decode_row_id(int row_id, int *row, int *col, int *digit) {
    *digit = row_id % 9;
    *col = (row_id / 9) % 9;
    *row = row_id / 81;
}

//Returns indices into 324 columns for: cell, row-digit, col-digit, box-digit
static inline void get_constraint_columns(int row, int col, int digit, int *cols) {
    int box = (row / BOX_SIZE) * BOX_SIZE + (col / BOX_SIZE);

    cols[0] = row * 9 + col;            //Cell constraint: 0-80
    cols[1] = 81 + row * 9 + digit;     //Row-number: 81-161
    cols[2] = 162 + col * 9 + digit;    //Col-number: 162-242
    cols[3] = 243 + box * 9 + digit;    //Box-number: 243-323
}

DLX *dlx_create(void);
void dlx_destroy(DLX *dlx);
bool dlx_search(DLX *dlx, int depth);
bool dlx_search_random(DLX *dlx, int depth);
void dlx_extract_grid(DLX *dlx, int depth, int grid[9][9]);
void dlx_print_solution_pretty(DLX *dlx, int depth);
void print_grid_pretty(int grid[9][9]);
bool sudoku_generate(int grid[9][9]);

#endif
