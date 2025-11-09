# Dancing Links (DLX) for Sudoku

A C implementation of Donald Knuth's "Dancing Links" algorithm (DLX). This project applies DLX to generate and solve 9x9 Sudoku puzzles.

Based on the 2000 paper: **[Dancing Links (arXiv:cs/0011047v1)](https://arxiv.org/abs/cs/0011047v1)**

## What's Implemented

* **Four-Way Linked List:** The core toroidal, `"four way linked"` list data structure described by Knuth.
* **Cover/Uncover Operations:** The fundamental `dlx_cover` and `dlx_uncover` operations that allow for efficient backtracking.
* **Algorithm X:** The recursive backtracking search (`dlx_search`) for finding exact covers.
* **S-Heuristic:** The "smallest column size" heuristic (`dlx_choose_column`) to prune the search tree effectively.
* **Sudoku Mapping:** Logic to map a 9x9 Sudoku grid to an exact cover problem with 324 constraints (Cell, Row-Digit, Col-Digit, and Box-Digit constraints).
* **Randomized Generator:** A randomized search (`dlx_search_random`) that shuffles row choices to efficiently generate a single, complete Sudoku grid.

## Usage

### Compile

```bash
gcc -o sudoku DancingLinksDS.c
```

### Run generator

``` bash
gcc DancingLinksDS.c -o sudoku
```

## Citation

```bash
@article{knuth2000dancing,
  title={Dancing links},
  author={Knuth, Donald E},
  journal={arXiv preprint cs/0011047},
  year={2000}
}
```
