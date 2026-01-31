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
   
    if (!rem) return -2; 
    return (rem && !(rem & (rem-1))) ? i : -1;
}

static inline void place(int i, int d) {
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
        if (ns == -2) { unsolved = -1; return; } //Finishes the curr loop before the main function notices?
        if (ns >= 0) naked[nc++] = ns;
    }
    
    for (int n = 0; n < nc; n++) {
        if (!cands[naked[n]]) continue;
        place(naked[n], __builtin_ctz(cands[naked[n]]));
    }
}

static void init_puzzle(const char *s) {
    unsolved = 81;
    
    memset(grid, 0, 81 * sizeof(u8));
    for (int i = 0; i < 81; i++) cands[i] = 0x1FF;   
 
    for (int d = 0; d < 9; d++) {
        for (int u = 0; u < 9; u++) {
            row_mask[d][u] = 0x1FF;
            col_mask[d][u] = 0x1FF;
            box_mask[d][u] = 0x1FF;
        }
    }
    
    for (int i = 0; i < 81; i++) {
        if (s[i] >= '1' && s[i] <= '9')
            place(i, s[i] - '1');
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

static int pointing(void) {
    int changed = 0;
    
    for (int d = 0; d < 9; d++) {
        int naked[27], nc = 0;
        
        for (int b = 0; b < 9; b++) {
            u16 m = box_mask[d][b];
            if (!m) continue;
            
            int br = (b / 3) * 3;
            int bc = (b % 3) * 3;
            
            if ((m & 0x007) == m || (m & 0x038) == m || (m & 0x1C0) == m) {
                int lr = (m & 0x007) ? 0 : (m & 0x038) ? 1 : 2;
                int r = br + lr;
                for (int c = 0; c < 9; c++) {
                    if (c / 3 == b % 3) continue;
                    int cell = r * 9 + c;
                    if (!(cands[cell] & (1 << d))) continue;
                    int ns = eliminate(cell, d);
                    if (ns == -2) { unsolved = -1; return 1; }
                    if (ns >= 0) naked[nc++] = ns;
                    changed = 1;
                }
            }
            
            if ((m & 0x049) == m || (m & 0x092) == m || (m & 0x124) == m) {
                int lc = (m & 0x049) ? 0 : (m & 0x092) ? 1 : 2;
                int c = bc + lc;
                for (int r = 0; r < 9; r++) {
                    if (r / 3 == b / 3) continue;
                    int cell = r * 9 + c;
                    if (!(cands[cell] & (1 << d))) continue;
                    int ns = eliminate(cell, d);
                    if (ns == -2) { unsolved = -1; return 1; }
                    if (ns >= 0) naked[nc++] = ns;
                    changed = 1;
                }
            }
        }
        
        for (int n = 0; n < nc; n++) {
            if (!cands[naked[n]]) continue;
            place(naked[n], __builtin_ctz(cands[naked[n]]));
            if (unsolved <= 0) return 1;
        }
    }
    
    return changed;
}

static int box_line(void) {
    int changed = 0;
    
    for (int d = 0; d < 9; d++) {
        int naked[18], nc = 0;
        
        for (int r = 0; r < 9; r++) {
            u16 m = row_mask[d][r];
            if (!m) continue;
            
            if ((m & 0x007) == m || (m & 0x038) == m || (m & 0x1C0) == m) {
                int bc = (m & 0x007) ? 0 : (m & 0x038) ? 3 : 6;
                int br = (r / 3) * 3;
                for (int rr = br; rr < br + 3; rr++) {
                    if (rr == r) continue;
                    for (int cc = bc; cc < bc + 3; cc++) {
                        int cell = rr * 9 + cc;
                        if (!(cands[cell] & (1 << d))) continue;
                        int ns = eliminate(cell, d);
                        if (ns == -2) { unsolved = -1; return 1; }
                        if (ns >= 0) naked[nc++] = ns;
                        changed = 1;
                    }
                }
            }
        }
        
        for (int c = 0; c < 9; c++) {
            u16 m = col_mask[d][c];
            if (!m) continue;
            
            if ((m & 0x007) == m || (m & 0x038) == m || (m & 0x1C0) == m) {
                int br = (m & 0x007) ? 0 : (m & 0x038) ? 3 : 6;
                int bc = (c / 3) * 3;
                for (int rr = br; rr < br + 3; rr++) {
                    for (int cc = bc; cc < bc + 3; cc++) {
                        if (cc == c) continue;
                        int cell = rr * 9 + cc;
                        if (!(cands[cell] & (1 << d))) continue;
                        int ns = eliminate(cell, d);
                        if (ns == -2) { unsolved = -1; return 1; }
                        if (ns >= 0) naked[nc++] = ns;
                        changed = 1;
                    }
                }
            }
        }
        
        for (int n = 0; n < nc; n++) {
            if (!cands[naked[n]]) continue;
            place(naked[n], __builtin_ctz(cands[naked[n]]));
            if (unsolved <= 0) return 1;
        }
    }
    
    return changed;
}

static int naked_pairs(void) {
    for (int r = 0; r < 9; r++) {
        int base = r * 9;
        for (int c1 = 0; c1 < 8; c1++) {
            int cell1 = base + c1;
            u16 m1 = cands[cell1];
            if (__builtin_popcount(m1) != 2) continue;
            for (int c2 = c1 + 1; c2 < 9; c2++) {
                if (cands[base + c2] != m1) continue;
                for (int c = 0; c < 9; c++) {
                    if (c == c1 || c == c2) continue;
                    int cell = base + c;
                    u16 elim = cands[cell] & m1;
                    if (!elim) continue;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        int ns = eliminate(cell, dd);
                        if (ns == -2) { unsolved = -1; return 1; }
                        if (ns >= 0) {
                            place(ns, __builtin_ctz(cands[ns]));
                            if (unsolved <= 0) return 1;
                        }
                    }
                    return 1;
                }
            }
        }
    }
    
    for (int c = 0; c < 9; c++) {
        for (int r1 = 0; r1 < 8; r1++) {
            int cell1 = r1 * 9 + c;
            u16 m1 = cands[cell1];
            if (__builtin_popcount(m1) != 2) continue;
            for (int r2 = r1 + 1; r2 < 9; r2++) {
                if (cands[r2 * 9 + c] != m1) continue;
                for (int r = 0; r < 9; r++) {
                    if (r == r1 || r == r2) continue;
                    int cell = r * 9 + c;
                    u16 elim = cands[cell] & m1;
                    if (!elim) continue;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        int ns = eliminate(cell, dd);
                        if (ns == -2) { unsolved = -1; return 1; }
                        if (ns >= 0) {
                            place(ns, __builtin_ctz(cands[ns]));
                            if (unsolved <= 0) return 1;
                        }
                    }
                    return 1;
                }
            }
        }
    }
    
    for (int b = 0; b < 9; b++) {
        for (int bp1 = 0; bp1 < 8; bp1++) {
            int cell1 = box_cell[b][bp1];
            u16 m1 = cands[cell1];
            if (__builtin_popcount(m1) != 2) continue;
            for (int bp2 = bp1 + 1; bp2 < 9; bp2++) {
                if (cands[box_cell[b][bp2]] != m1) continue;
                for (int bp = 0; bp < 9; bp++) {
                    if (bp == bp1 || bp == bp2) continue;
                    int cell = box_cell[b][bp];
                    u16 elim = cands[cell] & m1;
                    if (!elim) continue;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        int ns = eliminate(cell, dd);
                        if (ns == -2) { unsolved = -1; return 1; }
                        if (ns >= 0) {
                            place(ns, __builtin_ctz(cands[ns]));
                            if (unsolved <= 0) return 1;
                        }
                    }
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

static int hidden_pairs(void) {
    for (int r = 0; r < 9; r++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++)
            if (__builtin_popcount(row_mask[d][r]) == 2)
                valid[nv++] = d;
        for (int i = 0; i < nv; i++) {
            u16 m1 = row_mask[valid[i]][r];
            for (int j = i + 1; j < nv; j++) {
                if (row_mask[valid[j]][r] != m1) continue;
                u16 pair = (1 << valid[i]) | (1 << valid[j]);
                int found = 0;
                u16 m = m1;
                while (m) {
                    int c = __builtin_ctz(m);
                    m &= m - 1;
                    int cell = r * 9 + c;
                    u16 elim = cands[cell] & ~pair;
                    if (!elim) continue;
                    found = 1;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        if (eliminate(cell, dd) == -2) { unsolved = -1; return 1; }
                    }
                }
                if (found) return 1;
            }
        }
    }
    
    for (int c = 0; c < 9; c++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++)
            if (__builtin_popcount(col_mask[d][c]) == 2)
                valid[nv++] = d;
        for (int i = 0; i < nv; i++) {
            u16 m1 = col_mask[valid[i]][c];
            for (int j = i + 1; j < nv; j++) {
                if (col_mask[valid[j]][c] != m1) continue;
                u16 pair = (1 << valid[i]) | (1 << valid[j]);
                int found = 0;
                u16 m = m1;
                while (m) {
                    int r = __builtin_ctz(m);
                    m &= m - 1;
                    int cell = r * 9 + c;
                    u16 elim = cands[cell] & ~pair;
                    if (!elim) continue;
                    found = 1;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        if (eliminate(cell, dd) == -2) { unsolved = -1; return 1; }
                    }
                }
                if (found) return 1;
            }
        }
    }
    
    for (int b = 0; b < 9; b++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++)
            if (__builtin_popcount(box_mask[d][b]) == 2)
                valid[nv++] = d;
        for (int i = 0; i < nv; i++) {
            u16 m1 = box_mask[valid[i]][b];
            for (int j = i + 1; j < nv; j++) {
                if (box_mask[valid[j]][b] != m1) continue;
                u16 pair = (1 << valid[i]) | (1 << valid[j]);
                int found = 0;
                u16 m = m1;
                while (m) {
                    int bp = __builtin_ctz(m);
                    m &= m - 1;
                    int cell = box_cell[b][bp];
                    u16 elim = cands[cell] & ~pair;
                    if (!elim) continue;
                    found = 1;
                    while (elim) {
                        int dd = __builtin_ctz(elim);
                        elim &= elim - 1;
                        if (eliminate(cell, dd) == -2) { unsolved = -1; return 1; }
                    }
                }
                if (found) return 1;
            }
        }
    }
    
    return 0;
}

static int naked_triples(void) {
    for (int r = 0; r < 9; r++) {
        int base = r * 9;
        for (int c1 = 0; c1 < 7; c1++) {
            u16 m1 = cands[base + c1];
            int pc1 = __builtin_popcount(m1);
            if (pc1 < 2 || pc1 > 3) continue;
            for (int c2 = c1 + 1; c2 < 8; c2++) {
                u16 m2 = cands[base + c2];
                int pc2 = __builtin_popcount(m2);
                if (pc2 < 2 || pc2 > 3) continue;
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int c3 = c2 + 1; c3 < 9; c3++) {
                    u16 m3 = cands[base + c3];
                    int pc3 = __builtin_popcount(m3);
                    if (pc3 < 2 || pc3 > 3) continue;
                    u16 triple = u12 | m3;
                    if (__builtin_popcount(triple) != 3) continue;
                    for (int c = 0; c < 9; c++) {
                        if (c == c1 || c == c2 || c == c3) continue;
                        int cell = base + c;
                        u16 elim = cands[cell] & triple;
                        if (!elim) continue;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                        return 1;
                    }
                }
            }
        }
    }
    
    for (int c = 0; c < 9; c++) {
        for (int r1 = 0; r1 < 7; r1++) {
            u16 m1 = cands[r1 * 9 + c];
            int pc1 = __builtin_popcount(m1);
            if (pc1 < 2 || pc1 > 3) continue;
            for (int r2 = r1 + 1; r2 < 8; r2++) {
                u16 m2 = cands[r2 * 9 + c];
                int pc2 = __builtin_popcount(m2);
                if (pc2 < 2 || pc2 > 3) continue;
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int r3 = r2 + 1; r3 < 9; r3++) {
                    u16 m3 = cands[r3 * 9 + c];
                    int pc3 = __builtin_popcount(m3);
                    if (pc3 < 2 || pc3 > 3) continue;
                    u16 triple = u12 | m3;
                    if (__builtin_popcount(triple) != 3) continue;
                    for (int r = 0; r < 9; r++) {
                        if (r == r1 || r == r2 || r == r3) continue;
                        int cell = r * 9 + c;
                        u16 elim = cands[cell] & triple;
                        if (!elim) continue;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                        return 1;
                    }
                }
            }
        }
    }
    
    for (int b = 0; b < 9; b++) {
        for (int bp1 = 0; bp1 < 7; bp1++) {
            u16 m1 = cands[box_cell[b][bp1]];
            int pc1 = __builtin_popcount(m1);
            if (pc1 < 2 || pc1 > 3) continue;
            for (int bp2 = bp1 + 1; bp2 < 8; bp2++) {
                u16 m2 = cands[box_cell[b][bp2]];
                int pc2 = __builtin_popcount(m2);
                if (pc2 < 2 || pc2 > 3) continue;
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int bp3 = bp2 + 1; bp3 < 9; bp3++) {
                    u16 m3 = cands[box_cell[b][bp3]];
                    int pc3 = __builtin_popcount(m3);
                    if (pc3 < 2 || pc3 > 3) continue;
                    u16 triple = u12 | m3;
                    if (__builtin_popcount(triple) != 3) continue;
                    for (int bp = 0; bp < 9; bp++) {
                        if (bp == bp1 || bp == bp2 || bp == bp3) continue;
                        int cell = box_cell[b][bp];
                        u16 elim = cands[cell] & triple;
                        if (!elim) continue;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                        return 1;
                    }
                }
            }
        }
    }
    
    return 0;
}

static int hidden_triples(void) {
    for (int r = 0; r < 9; r++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++) {
            int pc = __builtin_popcount(row_mask[d][r]);
            if (pc >= 2 && pc <= 3)
                valid[nv++] = d;
        }
        if (nv < 3) continue;
        for (int i = 0; i < nv - 2; i++) {
            u16 m1 = row_mask[valid[i]][r];
            for (int j = i + 1; j < nv - 1; j++) {
                u16 m2 = row_mask[valid[j]][r];
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int k = j + 1; k < nv; k++) {
                    u16 m3 = row_mask[valid[k]][r];
                    u16 cells = u12 | m3;
                    if (__builtin_popcount(cells) != 3) continue;
                    u16 triple = (1 << valid[i]) | (1 << valid[j]) | (1 << valid[k]);
                    int found = 0;
                    u16 m = cells;
                    while (m) {
                        int c = __builtin_ctz(m);
                        m &= m - 1;
                        int cell = r * 9 + c;
                        u16 elim = cands[cell] & ~triple;
                        if (!elim) continue;
                        found = 1;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                    }
                    if (found) return 1;
                }
            }
        }
    }
    
    for (int c = 0; c < 9; c++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++) {
            int pc = __builtin_popcount(col_mask[d][c]);
            if (pc >= 2 && pc <= 3)
                valid[nv++] = d;
        }
        if (nv < 3) continue;
        for (int i = 0; i < nv - 2; i++) {
            u16 m1 = col_mask[valid[i]][c];
            for (int j = i + 1; j < nv - 1; j++) {
                u16 m2 = col_mask[valid[j]][c];
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int k = j + 1; k < nv; k++) {
                    u16 m3 = col_mask[valid[k]][c];
                    u16 cells = u12 | m3;
                    if (__builtin_popcount(cells) != 3) continue;
                    u16 triple = (1 << valid[i]) | (1 << valid[j]) | (1 << valid[k]);
                    int found = 0;
                    u16 m = cells;
                    while (m) {
                        int r = __builtin_ctz(m);
                        m &= m - 1;
                        int cell = r * 9 + c;
                        u16 elim = cands[cell] & ~triple;
                        if (!elim) continue;
                        found = 1;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                    }
                    if (found) return 1;
                }
            }
        }
    }
    
    for (int b = 0; b < 9; b++) {
        int valid[9], nv = 0;
        for (int d = 0; d < 9; d++) {
            int pc = __builtin_popcount(box_mask[d][b]);
            if (pc >= 2 && pc <= 3)
                valid[nv++] = d;
        }
        if (nv < 3) continue;
        for (int i = 0; i < nv - 2; i++) {
            u16 m1 = box_mask[valid[i]][b];
            for (int j = i + 1; j < nv - 1; j++) {
                u16 m2 = box_mask[valid[j]][b];
                u16 u12 = m1 | m2;
                if (__builtin_popcount(u12) > 3) continue;
                for (int k = j + 1; k < nv; k++) {
                    u16 m3 = box_mask[valid[k]][b];
                    u16 cells = u12 | m3;
                    if (__builtin_popcount(cells) != 3) continue;
                    u16 triple = (1 << valid[i]) | (1 << valid[j]) | (1 << valid[k]);
                    int found = 0;
                    u16 m = cells;
                    while (m) {
                        int bp = __builtin_ctz(m);
                        m &= m - 1;
                        int cell = box_cell[b][bp];
                        u16 elim = cands[cell] & ~triple;
                        if (!elim) continue;
                        found = 1;
                        while (elim) {
                            int dd = __builtin_ctz(elim);
                            elim &= elim - 1;
                            int ns = eliminate(cell, dd);
                            if (ns == -2) { unsolved = -1; return 1; }
                            if (ns >= 0) {
                                place(ns, __builtin_ctz(cands[ns]));
                                if (unsolved <= 0) return 1;
                            }
                        }
                    }
                    if (found) return 1;
                }
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
