# MazeLab — Testing Guide

## Summary

| Test Suite | Count | Result |
|-----------|-------|--------|
| C unit tests (algorithms, data structures) | 127 | ✅ All pass |
| PowerShell API integration tests | 47 | ✅ All pass |
| Manual browser verification | — | ✅ Verified |

---

## 1. C Unit Tests

### What is tested
- **Queue**: init, enqueue, dequeue, FIFO ordering, capacity limits, circular wraparound, overflow and underflow guards
- **Stack**: init, push, pop, LIFO ordering, capacity limits, overflow and underflow guards
- **Path reconstruction**: parent chain following, start==end edge case, unreachable cell (returns 0)
- **Maze validation**: dimension bounds, start/end on wall, in-bounds checks, adjacency function
- **BFS algorithm**: open grids, wall detours, unreachable destinations, start==end, linear mazes, repeated solves, multiple valid routes
- **DFS algorithm**: open grids, wall detours, unreachable destinations, start==end, linear forced paths
- **Trace**: exploration order starts at start, ends at destination, no duplicate first-visits in BFS

### Build and run
```bat
cd e:\DS-Microproject
mingw32-make test
```

### Expected output
```
========================================
  MazeLab Unit Tests
========================================

--- Queue ---
  PASS  queue empty on init
  ...
--- Stack ---
  ...
--- Path Reconstruction ---
  ...
--- Maze Validation ---
  ...
--- BFS Algorithm ---
  ...
--- DFS Algorithm ---
  ...
--- Trace ---
  ...
========================================
  Results: 127 passed, 0 failed
========================================
```

---

## 2. API Integration Tests

These tests verify the running HTTP server end-to-end.

### Prerequisites
1. Build the server: `mingw32-make all`
2. Start the server: `mazelab.exe` (or `run.bat`)
3. In a separate terminal, run:

```bat
powershell -ExecutionPolicy Bypass -File tests\test_api.ps1
```

### Test cases covered
| Group | Cases |
|-------|-------|
| Health endpoint | GET returns 200 + correct body fields; POST returns 405 |
| BFS solve | Returns 200, found, algorithm name, trace array, path array, visitedCount, pathLength, executionTimeUs; path starts at start, ends at end; pathLength = pathCellCount - 1 |
| DFS solve | Same correctness checks as BFS |
| Shortest path | BFS on open 3×3 returns pathCellCount=5, pathLength=4 |
| Unreachable | Returns 200, found=false, empty path |
| Start == End | found=true, pathCellCount=1, pathLength=0 |
| Maze generation | Returns rows, cols, grid array, start, end; start/end cells are open; generated maze is BFS-solvable |
| Error — malformed JSON | Returns 400 |
| Error — bad algorithm | Returns 400 |
| Error — invalid dimensions | Returns 400 |
| Error — start on wall | Returns 400 |
| Error — end on wall | Returns 400 |
| Error — missing fields | Returns 400 |
| Error — grid col mismatch | Returns 400 |
| Error — unknown route | Returns 404 |
| Error — wrong method (GET /api/solve) | Returns 405 |
| Error — wrong method (GET /api/maze/generate) | Returns 405 |

### Expected output
```
==========================================
  MazeLab API Integration Tests
  Target: http://127.0.0.1:8080
==========================================

--- Health endpoint ---
  PASS  GET /api/health returns 200
  PASS  health status=ok
  PASS  health app=MazeLab
  PASS  health version present
  PASS  POST /api/health returns 405

--- Solve: BFS ---
  PASS  BFS returns 200
  PASS  BFS found=true
  ...

==========================================
  Results: 47 passed, 0 failed
==========================================
```

---

## 3. Manual Browser Verification Checklist

After starting the server and opening `http://127.0.0.1:8080`:

### Connection
- [ ] Status indicator shows green dot "Connected — MazeLab 1.0.0"

### Default state
- [ ] 15×21 grid renders with start (green) at top-left and end (red) at bottom-right

### Wall drawing
- [ ] Click cells to draw/erase walls; drag to draw continuously
- [ ] Start and end cells cannot be walled over

### Start/End placement
- [ ] Select "Start" mode; click a cell — marker moves
- [ ] Select "End" mode; click a cell — marker moves
- [ ] Cannot place start on a wall or on the end cell

### BFS animation
- [ ] Select BFS; click Run
- [ ] Cells animate orange (current) → blue (explored) in breadth-first order
- [ ] Final path shows in bright orange
- [ ] Statistics panel shows visitedCount, pathCellCount, pathLength, executionTimeUs
- [ ] Explanation panel narrates each step

### DFS animation
- [ ] Select DFS; click Run
- [ ] Cells animate in depth-first order (noticeably different from BFS)
- [ ] A valid path is highlighted (possibly longer than BFS)

### Pause / Resume / Step
- [ ] Pause button pauses animation mid-sequence
- [ ] Resume continues
- [ ] Step button advances one cell at a time

### Speed control
- [ ] Slider from 10ms to 500ms changes animation speed visibly

### Compare
- [ ] Click "BFS vs DFS" — comparison table appears showing visited cells, path length, timing for both algorithms

### Unreachable maze
- [ ] Fill the row between start and end with walls; click Run
- [ ] Result: "No path found ✗", empty path array

### Generate
- [ ] Click "Generate" — new random maze appears with different wall layout

### Reset
- [ ] Click "Reset" — overlay clears, stats reset, ready to run again

### Keyboard
- [ ] Arrow keys move focus between cells; Enter/Space toggles wall under focused cell
- [ ] Space (not in input/button) triggers Run or Pause

---

## 4. Path Correctness Properties

Every path returned by the C backend is verified in unit tests to satisfy:

1. `path[0]` == start coordinate
2. `path[last]` == end coordinate
3. `|path[i].row - path[i+1].row| + |path[i].col - path[i+1].col| == 1` (orthogonal adjacency) for all i
4. `grid[path[i].row][path[i].col] == CELL_EMPTY` for all i
5. `pathLength == pathCellCount - 1`

BFS additionally satisfies: no other valid path has fewer cells (shortest path on unweighted grid).

---

## 5. Known Build Warnings (Non-Fatal)

When building on Windows with MinGW GCC 6.3, the following warnings appear in `mongoose.c`:

```
mongoose.c: warning: implicit declaration of function '_wfopen'
mongoose.c: warning: initialization makes pointer from integer without a cast
mongoose.c: warning: implicit declaration of function 'wcscat'
```

These are from Mongoose's internal Windows file-serving code and do not affect the HTTP API or pathfinding functionality. The server serves JSON responses correctly despite these warnings. They are caused by MinGW 6.3's incomplete `<wchar.h>` exposure under `-std=c11`.

---

*All tests pass on: Windows 11 + MinGW GCC 6.3.0 + mingw32-make*
