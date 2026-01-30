#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint16_t u16;
typedef uint8_t u8;

static u8 grid[81];
static u16 cands[81];
static u16 row_mask[9][9];
static u16 col_mask[9][9];
static u16 box_mask[9][9];
static int unsolved;

static u8 cell_box[81];
static u8 cell_boxpos[81];
static u8 cell_row[81];
static u8 cell_col[81];
static u8 peers[81][20];
static u8 box_cell[9][9];

static void init_tables(void) {
    for (int i = 0; i < 81; i++) {
        int r = i / 9, c = i % 9;
        cell_row[i] = r;
        cell_col[i] = c;
        cell_box[i] = (r / 3) * 3 + c / 3;
        cell_boxpos[i] = (r % 3) * 3 + c % 3;
        
        int p = 0;
        for (int cc = 0; cc < 9; cc++)
            if (cc != c) peers[i][p++] = r * 9 + cc;
        for (int rr = 0; rr < 9; rr++)
            if (rr != r) peers[i][p++] = rr * 9 + c;
        int br = (r / 3) * 3, bc = (c / 3) * 3;
        for (int dr = 0; dr < 3; dr++)
            for (int dc = 0; dc < 3; dc++) {
                int rr = br + dr, cc = bc + dc;
                if (rr != r && cc != c) peers[i][p++] = rr * 9 + cc;
            }
    }
    for (int b = 0; b < 9; b++)
        for (int bp = 0; bp < 9; bp++)
            box_cell[b][bp] = (b/3)*27 + (bp/3)*9 + (b%3)*3 + bp%3;
}

static inline int eliminate(int i, int d) {
    u16 m = 1 << d;
    u16 old = cands[i];
    if (!(old & m)) return -1;
    
    u16 rem = old ^ m;
    cands[i] = rem;
    
    int r = cell_row[i], c = cell_col[i];
    row_mask[d][r] &= ~(1 << c);
    col_mask[d][c] &= ~(1 << r);
    box_mask[d][cell_box[i]] &= ~(1 << cell_boxpos[i]);
    
    return (rem && !(rem & (rem-1))) ? i : -1;
}

static void place(int i, int d) {
    int r = cell_row[i], c = cell_col[i], b = cell_box[i], bp = cell_boxpos[i];
    
    for (int dd = 0; dd < 9; dd++) {
        row_mask[dd][r] &= ~(1 << c);
        col_mask[dd][c] &= ~(1 << r);
        box_mask[dd][b] &= ~(1 << bp);
    }
    
    row_mask[d][r] = 0;
    col_mask[d][c] = 0;
    box_mask[d][b] = 0;
    
    grid[i] = d + 1;
    cands[i] = 0;
    unsolved--;
    
    int naked[20];
    int nc = 0;
    
    for (int p = 0; p < 20; p++) {
        int ns = eliminate(peers[i][p], d);
        if (ns >= 0) naked[nc++] = ns;
    }
    
    for (int n = 0; n < nc; n++) {
        if (!cands[naked[n]]) continue;
        place(naked[n], __builtin_ctz(cands[naked[n]]));
    }
}

static void init_puzzle(const char *s) {
    unsolved = 0;
    memset(row_mask, 0, sizeof(row_mask));
    memset(col_mask, 0, sizeof(col_mask));
    memset(box_mask, 0, sizeof(box_mask));
    
    for (int i = 0; i < 81; i++) {
        if (s[i] >= '1' && s[i] <= '9') {
            grid[i] = s[i] - '0';
            cands[i] = 0;
        } else {
            grid[i] = 0;
            cands[i] = 0x1FF;
            unsolved++;
        }
    }
    
    for (int i = 0; i < 81; i++) {
        if (!grid[i]) continue;
        int d = grid[i] - 1;
        for (int p = 0; p < 20; p++)
            cands[peers[i][p]] &= ~(1 << d);
    }
    
    for (int i = 0; i < 81; i++) {
        if (grid[i]) continue;
        int r = cell_row[i], c = cell_col[i], b = cell_box[i], bp = cell_boxpos[i];
        for (int d = 0; d < 9; d++) {
            if (cands[i] & (1 << d)) {
                row_mask[d][r] |= (1 << c);
                col_mask[d][c] |= (1 << r);
                box_mask[d][b] |= (1 << bp);
            }
        }
    }
}

static int hidden_single(void) {
    for (int d = 0; d < 9; d++) {
        for (int r = 0; r < 9; r++) {
            u16 m = row_mask[d][r];
            if (m && !(m & (m-1))) {
                place(r * 9 + __builtin_ctz(m), d);
                return 1;
            }
        }
        for (int c = 0; c < 9; c++) {
            u16 m = col_mask[d][c];
            if (m && !(m & (m-1))) {
                place(__builtin_ctz(m) * 9 + c, d);
                return 1;
            }
        }
        for (int b = 0; b < 9; b++) {
            u16 m = box_mask[d][b];
            if (m && !(m & (m-1))) {
                place(box_cell[b][__builtin_ctz(m)], d);
                return 1;
            }
        }
    }
    return 0;
}

int main(void) {
    init_tables();
    
    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        if (strlen(line) < 81) continue;
        
        init_puzzle(line);
        while (unsolved > 0 && hidden_single());
        
        for (int i = 0; i < 81; i++)
            putchar(grid[i] ? '0' + grid[i] : '.');
        putchar('\n');
    }
    return 0;
}
