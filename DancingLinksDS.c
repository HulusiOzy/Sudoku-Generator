#include "DancingLinksDS.h"

#include <limits.h>
#include <time.h>
#include <stdbool.h> 

static void add_row(DLX *dlx, ColumnHeader **columns, int row, int col, int digit);

//  Col header init/creator
static ColumnHeader *create_column_header(int index) {
    ColumnHeader *col = (ColumnHeader *)malloc(sizeof(ColumnHeader));
    if (!col) {
        fprintf(stderr, "Failed to allocate column header \n");
        exit(1);
    }

    //  Init embedded node
    col->node.left = col->node.right = &(col->node);
    col->node.up = col->node.down = &(col->node);
    col->node.column = col;
    col->node.row_id = -1;  //Col headers dont represent rows
    
    col->size = 0;
    snprintf(col->name, sizeof(col->name), "C%d", index);

    return col;
}

// Node init/creator
static Node *create_node(int row_id, ColumnHeader *column) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) {
        fprintf(stderr, "Failed to allocate node \n");
        exit(1);
    }

    node->row_id = row_id;
    node->column = column;
    node->left = node->right = node->up = node->down = node;

    return node;
}

DLX *dlx_create(void) {
    //Allocate the main DLX structure
    DLX *dlx = (DLX *)malloc(sizeof(DLX));
    if (!dlx){
        fprintf(stderr, "Failed to allocate DLX structure \n");
        exit(1);
    }

    //Init solution tracking
    dlx->solution_size = 0;
    dlx->solutions_found = 0;
    dlx->max_solution_size = 81; //Max for sudoku
    dlx->solution = (int *)malloc(81 * sizeof(int));
    if (!dlx->solution) {
        fprintf(stderr, "Failed to allocate solution array \n");
        free(dlx);
        exit(1);
    }

    //Create root header
    dlx->root = create_column_header(-1);
    strcpy(dlx->root->name, "ROOT");

    //Create array of 324 column headers
    ColumnHeader **columns = (ColumnHeader **)malloc(NUM_CONSTRAINTS * sizeof(ColumnHeader *));
    if (!columns) {
        fprintf(stderr, "Failed to allocate columns array \n");
        free(dlx->solution);
        free(dlx->root);
        free(dlx);
        exit(1);
    }

    //Create all 324 column headers and link them
    for (int i = 0; i < NUM_CONSTRAINTS; i++) {
        columns[i] = create_column_header(i);

        ColumnHeader *last = (ColumnHeader *)dlx->root->node.left;
        columns[i]->node.left = &(last->node);
        columns[i]->node.right = &(dlx->root->node);
        last->node.right = &(columns[i]->node);
        dlx->root->node.left = &(columns[i]->node);
    }

    //Create all 729 possible rows
    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N; col++) {
            for (int digit = 0; digit < N; digit++) {
                add_row(dlx, columns, row, col, digit);
            }
        }
    }

    free(columns);
    return dlx;
}

static void add_row(DLX *dlx, ColumnHeader **columns, int row, int col, int digit) {
    int row_id = encode_row_id(row, col, digit);

    //Get 4 constraint cols that this placement satisfies
    int constraint_cols[4];
    get_constraint_columns(row, col, digit, constraint_cols);

    //Create 4 nodes, one for each constraint
    Node *nodes[4];

    for (int i = 0; i < 4; i++) {
        int  col_idx = constraint_cols[i];
        nodes[i] = create_node(row_id, columns[col_idx]);

        //Link vertically to col
        ColumnHeader *col_header = columns[col_idx];
        Node *last = col_header->node.up;

        nodes[i]->up = last;
        nodes[i]->down = &(col_header->node);
        last->down = nodes[i];
        col_header->node.up = nodes[i];

        col_header->size++;
    }

    //Link horizontally
    for (int i = 0; i < 4; i++) {
            nodes[i]->left = nodes[(i + 3) % 4];
            nodes[i]->right = nodes[(i+1) % 4];
    }
}

//
//The steps of the dance
//

static void dlx_cover(ColumnHeader *col) {
    //Remove col header list(horizontal removal!)
    Node *col_node = &(col->node);
    col_node->right->left = col_node->left;
    col_node->left->right = col_node->right;

    //Remove all rows that intersect this col
    for (Node *row_node = col_node->down;
            row_node != col_node;
            row_node = row_node->down) {
        
        //For each node in row, other than one in col
        for (Node *right_node = row_node->right;
                right_node != row_node;
                right_node = right_node->right) {
            
            //Remove right_node from its column(vertical)
            right_node->down->up = right_node->up;
            right_node->up->down = right_node->down;
            right_node->column->size--;
        }
    }
}

static void dlx_uncover(ColumnHeader *col) {
    Node *col_node = &(col->node);

    //Restore rows, MUST BE REVERSE ORDER/bottom to top
    for (Node *row_node = col_node->up;
            row_node != col_node;
            row_node = row_node->up) {

        //For each node in row, go left OPPOSITE of cover
        for (Node *left_node = row_node->left;
                left_node != row_node;
                left_node = left_node->left) {

            //Restore left_node to its column/vertical
            left_node->column->size++;
            left_node->down->up = left_node;
            left_node->up->down = left_node;
        }
    }

    //Restore column to header list/horizontal
    col_node->right->left = col_node;
    col_node->left->right = col_node;
}

static ColumnHeader *dlx_choose_column(DLX *dlx) {
    //Heuristic, choose col with min size
    ColumnHeader *best_col = NULL;
    int min_size = INT_MAX;

    //Traverse the circular header list
    for (Node *col_node = dlx->root->node.right;
            col_node != &(dlx->root->node);
            col_node = col_node->right) {

        ColumnHeader *col = (ColumnHeader *)col_node;

        if (col->size < min_size) {
            min_size = col->size;
            best_col = col;
        }
    }

    return best_col;
}

bool dlx_search(DLX *dlx, int depth) {
    //Base case: all cols covered - we got a solution
    if(dlx->root->node.right == &(dlx->root->node)) {
        dlx->solutions_found++;
        return true;
    }

    //Choose column with min size(heuristic)
    ColumnHeader *col = dlx_choose_column(dlx);

    //Dead end: Col has no rows
    if (col->size == 0) {
        return false;
    }

    //Cover this col
    dlx_cover(col);

    //Try each row in this col
    for (Node *row_node = col->node.down;
            row_node != &(col->node);
            row_node = row_node->down) {

        //Add row to sol
        dlx->solution[depth] = row_node->row_id;

        //Cover the other cols in this row
        for (Node *right_node = row_node->right;
                right_node != row_node;
                right_node = right_node->right) {

            dlx_cover(right_node->column);
        }

        //Recurse
        dlx_search(dlx, depth + 1);

        //Backtrack: uncover in reverse
        for (Node *left_node = row_node-> left;
                left_node != row_node;
                left_node = left_node->left) {
            
            dlx_uncover(left_node->column);
        }
    }

    //Uncover the chosen col
    dlx_uncover(col);
    return false;
}

//Fisher-Yates shuffle
static void shuffle_nodes(Node **array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i+1);
        Node *temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

bool dlx_search_random(DLX *dlx, int depth) {
    //Base case: all cols covered - we got a solution
    if(dlx->root->node.right == &(dlx->root->node)) {
        dlx->solutions_found++;
        return true;
    }

    //Choose column with min size(heuristic)
    ColumnHeader *col = dlx_choose_column(dlx);

    //Dead end: Col has no rows
    if (col->size == 0) {
        return false;
    }
    
    //Collect all rows in this col into an arr
    int num_rows = col->size;
    Node **rows = (Node **)malloc(num_rows * sizeof(Node *));
    if (!rows) {
        fprintf(stderr, "Failed to allocate rows array \n");
        return false;
    }
    
    int idx = 0;
    for (Node *row_node = col->node.down;
            row_node != &(col->node);
            row_node = row_node->down) {
        rows[idx++] = row_node;
    }
    
    //Shuffle array
    shuffle_nodes(rows, num_rows);

    //Try rows in shuffled order
    dlx_cover(col);

    bool found = false;
    for (int i = 0; i < num_rows; i++) {
        Node *row_node = rows[i];
        dlx->solution[depth] = row_node->row_id;

        //Cover other cols in this row
        for (Node *right_node = row_node->right;
                right_node != row_node;
                right_node = right_node->right) {
            dlx_cover(right_node->column);
        }

        //Recurse
        if (dlx_search_random(dlx, depth + 1)) {
            found = true;
            free(rows);
            dlx_uncover(col);
            return true;    //Return on first sol
        }

        //Backtrack
        for(Node *left_node = row_node->left;
                left_node != row_node;
                left_node = left_node->left) {
            dlx_uncover(left_node->column);
        }
    }
    
    dlx_uncover(col);
    free(rows);
    return found;
}

void dlx_destroy(DLX *dlx){
    if (!dlx) return;

    //Free all nodes in all cols!
    for (Node *col_node = dlx->root->node.right;
            col_node != &(dlx->root->node);
            /* dont advance here, we are deleting*/) {

        ColumnHeader *col = (ColumnHeader *)col_node;
        Node *next_col = col_node->right; //Save next before del

        //Free all nodes in this col
        Node *node = col->node.down;
        while (node != &(col->node)) {
            Node *next_node = node->down;
            free(node); //Free the node
            node = next_node;
        }

        //Free the header
        free(col);

        col_node = next_col; //Move to next col
    }

    //Free root, sol array, dlx struct
    free(dlx->root);
    free(dlx->solution);
    free(dlx);
}

void dlx_extract_grid(DLX *dlx, int depth, int grid[9][9]) {
    //Init grid to 0
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            grid[r][c] = 0;
        }
    }

    //Fill in the solution
    for (int i = 0; i < depth; i++) {
        int row, col, digit;
        decode_row_id(dlx->solution[i], &row, &col, &digit);
        grid[row][col] = digit + 1;  //Convert 0-based to 1-9
    }
}

void dlx_print_solution_pretty(DLX *dlx, int depth) {
    int grid[9][9];
    dlx_extract_grid(dlx, depth, grid);

    printf("Solution #%d:\n", dlx->solutions_found);
    for (int r = 0; r < 9; r++) {
        if (r % 3 == 0 && r != 0) {
            printf("------+-------+------\n");
        }
        for (int c = 0; c < 9; c++) {
            if (c % 3 == 0 && c != 0) {
                printf("| ");
            }
            printf("%d ", grid[r][c]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_grid_pretty(int grid[9][9]) {
    for (int r = 0; r < 9; r++) {
        if (r % 3 == 0 && r != 0) {
            printf("------+-------+------\n");
        }
        for (int c = 0; c < 9; c++) {
            if (c % 3 == 0 && c != 0) {
                printf("| ");
            }
            printf("%d ", grid[r][c]);
        }
        printf("\n");
    }
}

//High levels
bool sudoku_generate(int grid[9][9]) {
    DLX *dlx = dlx_create();
    bool found = dlx_search_random(dlx, 0);

    if (found) {
        dlx_extract_grid(dlx, 81, grid);
    }

    dlx_destroy(dlx);
    return found;
}

int main(void) {
    srand(time(NULL));

    int grid[9][9];
    
    if (sudoku_generate(grid)) {
        print_grid_pretty(grid); 
    } else {
        printf("Failed to generate \n");
    }

    return 0;
}
