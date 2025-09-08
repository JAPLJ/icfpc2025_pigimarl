# ICFPC 2025 Pigimarl

## Programs

### japlj/ (Rust)
- Main solver implementation (all scores are obtained with these solvers)
- Several solvers with different approaches:
  - (Lightning) `solver1.rs`: Random walk to identify nodes + edge identification
  - (Lightning)`solver2.rs`: CSP encoding with Z3 solver
  - `solver3.rs`: Random walk to identify nodes + edge identification (for Full Division)
  - `solver4.rs`: Improved version of solver3 (can now handle all problems)
  - `solver5.rs`: Backtracking with constraint solving (designed for identifying graphs with 1 query)
  - `solver6.rs`: Improved version of solver5 (works for n ≤ 18)

### tester/ (Rust)
- Local testing server that simulates the competition API
