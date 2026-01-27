#include "DancingLinksDS.h"

#include <limits.h>
#include <time.h>
#include <stdbool.h> 

static void link_row(DLX *dlx, int row, int col, int digit); 

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

void dlx_reset(DLX *dlx) {
    // Reset root's horizontal links
    dlx->root->node.left = &dlx->root->node;
    dlx->root->node.right = &dlx->root->node;

    // Reset all columns and re-link to root
    for (int i = 0; i < NUM_CONSTRAINTS; i++) {
        ColumnHeader *col = dlx->columns[i];
        
        // Reset vertical links to self
        col->node.up = &col->node;
        col->node.down = &col->node;
        col->size = 0;

        // Re-link horizontally to root
        ColumnHeader *last = (ColumnHeader *)dlx->root->node.left;
        col->node.left = &last->node;
        col->node.right = &dlx->root->node;
        last->node.right = &col->node;
        dlx->root->node.left = &col->node;
    }

    // Re-link all rows
    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N; col++) {
            for (int digit = 0; digit < N; digit++) {
                link_row(dlx, row, col, digit);
            }
        }
    }

    dlx->solutions_found = 0;
    dlx->solution_size = 0;
}

DLX *dlx_create(void) {
    DLX *dlx = (DLX *)malloc(sizeof(DLX));
    if (!dlx) {
        fprintf(stderr, "Failed to allocate DLX structure\n");
        exit(1);
    }

    dlx->solution_size = 0;
    dlx->solutions_found = 0;
    dlx->max_solution_size = 81;
    dlx->solution = (int *)malloc(81 * sizeof(int));
    if (!dlx->solution) {
        fprintf(stderr, "Failed to allocate solution array\n");
        free(dlx);
        exit(1);
    }

    // Allocate flat node array
    dlx->all_nodes = (Node *)malloc(MAX_ROWS * 4 * sizeof(Node));
    if (!dlx->all_nodes) {
        fprintf(stderr, "Failed to allocate nodes array\n");
        free(dlx->solution);
        free(dlx);
        exit(1);
    }

    dlx->root = create_column_header(-1);
    strcpy(dlx->root->name, "ROOT");

    dlx->columns = (ColumnHeader **)malloc(NUM_CONSTRAINTS * sizeof(ColumnHeader *));
    if (!dlx->columns) {
        fprintf(stderr, "Failed to allocate columns array\n");
        free(dlx->all_nodes);
        free(dlx->solution);
        free(dlx->root);
        free(dlx);
        exit(1);
    }

    // Create column headers (one-time allocation)
    for (int i = 0; i < NUM_CONSTRAINTS; i++) {
        dlx->columns[i] = create_column_header(i);
    }

    // Use reset to wire everything
    dlx_reset(dlx);

    return dlx;
}

static void link_row(DLX *dlx, int row, int col, int digit) {
    int row_id = encode_row_id(row, col, digit);
    int node_base = row_id * 4;

    int constraint_cols[4];
    get_constraint_columns(row, col, digit, constraint_cols);

    for (int i = 0; i < 4; i++) {
        Node *node = &dlx->all_nodes[node_base + i];
        ColumnHeader *col_header = dlx->columns[constraint_cols[i]];

        node->row_id = row_id;
        node->column = col_header;

        // Link vertically
        Node *last = col_header->node.up;
        node->up = last;
        node->down = &col_header->node;
        last->down = node;
        col_header->node.up = node;

        col_header->size++;
    }

    // Link horizontally
    for (int i = 0; i < 4; i++) {
        Node *node = &dlx->all_nodes[node_base + i];
        node->left = &dlx->all_nodes[node_base + ((i + 3) % 4)];
        node->right = &dlx->all_nodes[node_base + ((i + 1) % 4)];
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
        if (dlx_search(dlx, depth + 1)) {
            dlx_uncover(col);
            return true;
        }

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
    return false;
}

void dlx_destroy(DLX *dlx) {
    if (!dlx) return;

    for (int i = 0; i < NUM_CONSTRAINTS; i++) {
        free(dlx->columns[i]);
    }

    free(dlx->columns);
    free(dlx->all_nodes);
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

//Surely theres a smart way of doing these rather than seperate funcs, but eh, im not a good enough programmer to know
void dlx_apply_clue(DLX *dlx, int row, int col, int digit){
    int constraint_cols[4];
    get_constraint_columns(row, col, digit, constraint_cols);
    
    for (int i = 0; i < 4; i++){
        dlx_cover(dlx->columns[constraint_cols[i]]);
    }
}

void dlx_remove_clue(DLX *dlx, int row, int col, int digit) {
    int constraint_cols[4];
    get_constraint_columns(row, col, digit, constraint_cols);
    
    for (int i = 3; i >= 0; i--) {
        dlx_uncover(dlx->columns[constraint_cols[i]]);
    }
}



//Caller needs to reset dlx->solutions_found = 0 before calling. Didnt bake in because it seemd like a good design decision at the time.
int dlx_count_solutions(DLX *dlx, int depth, int max_count) {
    //Base case where all columns are covered, we have a solution
    if (dlx->root->node.right == &(dlx->root->node)) {
        dlx->solutions_found++;
        return dlx->solutions_found;
    }

    //Choose a col with min size
    ColumnHeader *col = dlx_choose_column(dlx);

    //Dead end
    if (col->size == 0){
        return dlx->solutions_found;
    }

    dlx_cover(col);

    for (Node *row_node = col->node.down;
            row_node != &(col->node);
            row_node = row_node->down) {
     
       dlx->solution[depth] = row_node->row_id;

        for (Node *right_node = row_node->right;
                right_node != row_node;
                right_node = right_node->right) {
            dlx_cover(right_node->column);
        }

        dlx_count_solutions(dlx, depth + 1, max_count);

        //Early exit if we've hit max
        if (dlx->solutions_found >= max_count) {
            //Uncover before bailing
            for (Node *left_node = row_node->left;
                    left_node != row_node;
                    left_node = left_node->left) {
                dlx_uncover(left_node->column);
            }
            dlx_uncover(col);
            return dlx->solutions_found;
        }

        //Backtrack
        for (Node *left_node = row_node->left;
                left_node != row_node;
                left_node = left_node->left) {
            dlx_uncover(left_node->column);
        }
    }

    dlx_uncover(col);
    return dlx->solutions_found;
}

int sudoku_create_puzzle(int full_grid[9][9], int puzzle_grid[9][9]) {
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            puzzle_grid[r][c] = full_grid[r][c];
        }
    }

    int positions[81];
    for (int i = 0; i < 81; i++) {
        positions[i] = i;
    }
    for (int i = 80; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = positions[i];
        positions[i] = positions[j];
        positions[j] = temp;
    }

    DLX *dlx = dlx_create();  // One allocation
    int clues_remaining = 81;

    for (int i = 0; i < 81; i++) {
        int pos = positions[i];
        int r = pos / 9;
        int c = pos % 9;

        if (puzzle_grid[r][c] == 0) continue;

        int saved_digit = puzzle_grid[r][c];
        puzzle_grid[r][c] = 0;

        dlx_reset(dlx);  // Fast pointer rewiring, no malloc

        for (int rr = 0; rr < 9; rr++) {
            for (int cc = 0; cc < 9; cc++) {
                if (puzzle_grid[rr][cc] != 0) {
                    dlx_apply_clue(dlx, rr, cc, puzzle_grid[rr][cc] - 1);
                }
            }
        }

        dlx->solutions_found = 0;
        int count = dlx_count_solutions(dlx, 0, 2);

        if (count >= 2) {
            puzzle_grid[r][c] = saved_digit;
        } else {
            clues_remaining--;
        }
    }

    dlx_destroy(dlx);  // One free
    return clues_remaining;
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

    int full_grid[9][9];
    int puzzle_grid[9][9];
    
    if (sudoku_generate(full_grid)) {
        printf("Full solution:\n");
        print_grid_pretty(full_grid);
        
        int clues = sudoku_create_puzzle(full_grid, puzzle_grid);
        
        printf("\nPuzzle (%d clues):\n", clues);
        print_grid_pretty(puzzle_grid);
    } else {
        printf("Failed to generate\n");
    }

    return 0;
}
