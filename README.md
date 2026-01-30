# Dancing Links (DLX) for Sudoku

A C implementation of Donald Knuth's "Dancing Links" algorithm (DLX). This project applies DLX to generate and solve 9x9 Sudoku puzzles.

Based on the 2000 paper: **[Dancing Links (arXiv:cs/0011047v1)](https://arxiv.org/abs/cs/0011047v1)**

## Usage

### Compile

```bash
gcc -o sudoku DancingLinksDS.c
```

### Run generator

``` bash
./sudoku
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
