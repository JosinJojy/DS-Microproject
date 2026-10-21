# MazeLab — Interactive Maze Solver & Data Structures Visualizer

A complete educational web application demonstrating how **BFS** (Breadth-First Search) and **DFS** (Depth-First Search) solve mazes. The algorithms run entirely in **C**, served over a real HTTP API. The browser only visualizes the result — it never computes paths itself.

---

## Table of Contents
1. [Project Overview](#1-project-overview)
2. [Problem Statement](#2-problem-statement)
3. [Objectives](#3-objectives)
4. [Features](#4-features)
5. [Technologies Used](#5-technologies-used)
6. [Architecture](#6-architecture)
7. [Folder Structure](#7-folder-structure)
8. [Data Structures Used](#8-data-structures-used)
9. [BFS Algorithm](#9-bfs-algorithm)
10. [DFS Algorithm](#10-dfs-algorithm)
11. [Queue and Stack Implementations](#11-queue-and-stack-implementations)
12. [Parent Tracking and Path Reconstruction](#12-parent-tracking-and-path-reconstruction)
13. [API Routes](#13-api-routes)
14. [Installation Prerequisites](#14-installation-prerequisites)
15. [Run Instructions](#15-run-instructions)
16. [Build and Test Commands](#16-build-and-test-commands)
17. [Expected Behaviour](#17-expected-behaviour)
18. [Complexity Analysis](#18-complexity-analysis)
19. [Limitations](#19-limitations)
20. [Third-Party Attribution](#20-third-party-attribution)
21. [Troubleshooting](#21-troubleshooting)
22. [Viva Preparation](#22-viva-preparation)

---

## 1. Project Overview

**MazeLab** is a locally-runnable, self-contained application that:
- Lets you draw custom mazes in the browser
- Runs BFS or DFS on the C backend and streams the result back
- Animates the actual exploration trace, then highlights the final path
- Compares BFS and DFS side-by-side on the same maze
- Works completely offline after the initial build

---

## 2. Problem Statement

Pathfinding in a grid maze is a classic graph-traversal problem. Given a grid of traversable (empty) and impassable (wall) cells, a start cell, and a destination cell, find a valid route or determine that none exists. Different traversal strategies (BFS vs. DFS) find paths differently, and understanding those differences is a core data-structures concept.

---

## 3. Objectives

- Implement a real FIFO queue and LIFO stack in C
- Implement iterative BFS and DFS using those structures
- Expose the algorithm results over a real HTTP JSON API
- Visualize the exploration order and final path in a browser
- Allow a student to observe, explain, and compare the two algorithms

---

## 4. Features

| Feature | Status |
|---------|--------|
| BFS pathfinding (C) | ✅ |
| DFS pathfinding (C, iterative) | ✅ |
| Circular array queue | ✅ |
| Array-backed stack | ✅ |
| Path reconstruction via parent array | ✅ |
| Exploration trace animation | ✅ |
| Pause / Resume / Step controls | ✅ |
| Animation speed control | ✅ |
| BFS vs DFS comparison | ✅ |
| Random maze generation (C, recursive backtracker) | ✅ |
| Wall drawing by click and drag | ✅ |
| Start / End cell placement | ✅ |
| Statistics panel (visited, path length, time) | ✅ |
| Backend health indicator | ✅ |
| Unreachable destination handling | ✅ |
| Start == End case | ✅ |
| C unit tests (127 cases) | ✅ |
| PowerShell API integration tests | ✅ |
| Responsive layout | ✅ |
| Keyboard navigation | ✅ |
| Reduced-motion support | ✅ |
| Works offline after build | ✅ |

---

## 5. Technologies Used

| Layer | Technology | Reason |
|-------|-----------|--------|
| Backend language | C11 (GCC / MinGW) | Efficient, educational, no runtime dependencies |
| HTTP server | Mongoose 7.14 (vendored) | Single-file C HTTP library, no install needed |
| JSON library | cJSON 1.7.18 (vendored) | Lightweight C JSON parser |
| Build system | Makefile + mingw32-make | Standard, simple, no extra tooling |
| Frontend | HTML5 + CSS3 + Vanilla JS | No frameworks, clear code for students |
| Font | System font stack | No remote resources required |

---

## 6. Architecture

```
Browser (frontend/)
    │
    │  HTTP JSON requests (Fetch API)
    ▼
C HTTP Server (Mongoose, port 8080)
    │
    ├── GET  /api/health          → server status
    ├── POST /api/solve           → BFS or DFS, returns trace + path
    ├── POST /api/maze/generate   → random maze (recursive backtracker)
    └── GET  /*                  → serves frontend/ static files
         │
         └── C modules:
               maze.c    – grid, validation
               queue.c   – circular FIFO queue
               stack.c   – array LIFO stack
               bfs.c     – BFS algorithm
               dfs.c     – DFS algorithm
               path.c    – parent array, path reconstruction
               api.c     – HTTP handlers, JSON encode/decode
```

**Key rule:** JavaScript never computes paths. It only *replays* the trace returned by C.

---

## 7. Folder Structure

```
DS-Microproject/
├── backend/
│   ├── main.c          Entry point; Mongoose event loop
│   ├── api.c/h         HTTP route handlers; JSON serialization
│   ├── maze.c/h        Grid struct; validation; cell types
│   ├── queue.c/h       Circular array FIFO queue (for BFS)
│   ├── stack.c/h       Array-backed LIFO stack (for DFS)
│   ├── bfs.c/h         Breadth-First Search implementation
│   ├── dfs.c/h         Depth-First Search implementation (iterative)
│   └── path.c/h        Parent grid; path reconstruction; trace
├── frontend/
│   ├── index.html      Semantic HTML; all UI elements
│   ├── styles.css      Dark editorial design; CSS variables
│   ├── api.js          Fetch wrappers; timeout; request ID tracking
│   ├── renderer.js     Grid DOM rendering; event delegation; overlay
│   ├── visualizer.js   Animation playback; step controls; speed
│   └── app.js          App state; event coordination; comparison
├── third_party/
│   ├── mongoose/       mongoose.h + mongoose.c (v7.14)
│   └── cjson/          cJSON.h + cJSON.c (v1.7.18)
├── tests/
│   ├── test_algorithms.c   C unit tests (127 test cases)
│   ├── test_api.ps1        PowerShell API integration tests
│   └── test_api.sh         Shell API integration tests (Linux/macOS)
├── Makefile            Build rules: all, test, debug, clean
├── run.bat             Windows one-command launcher
├── run.sh              Linux/macOS launcher
├── README.md           This file
├── TESTING.md          Testing guide and results
└── LICENSES.md         Third-party license notices
```

---

## 8. Data Structures Used

### 8.1 Maze Grid (`maze.h`)
A fixed-size 2-D array `uint8_t grid[MAZE_MAX_ROWS][MAZE_MAX_COLS]`.  
Each cell is `CELL_EMPTY (0)` or `CELL_WALL (1)`.  
Start and end are stored as separate `Coord {row, col}` — they are never encoded as special grid values.

**Why:** A 2-D array gives O(1) lookup by coordinate, which is needed thousands of times during BFS/DFS.

### 8.2 Circular Array Queue (`queue.h`)
Used by BFS. Internally: `Coord data[QUEUE_CAPACITY]`, `int head, tail, count`.

- Enqueue: write to `data[tail]`, advance `tail = (tail+1) % CAPACITY`
- Dequeue: read from `data[head]`, advance `head = (head+1) % CAPACITY`
- The modulo wrap-around avoids shifting elements — all operations are **O(1)**.

### 8.3 Array-Backed Stack (`stack.h`)
Used by DFS. Internally: `Coord data[STACK_CAPACITY]`, `int top` (−1 when empty).

- Push: `data[++top] = coord`
- Pop: `return data[top--]`
- All operations are **O(1)**.

### 8.4 Parent Grid (`path.h`)
A `Coord parent[MAZE_MAX_ROWS][MAZE_MAX_COLS]`.  
When cell B is first discovered from A, we set `parent[B.row][B.col] = A`.  
Unvisited cells hold the sentinel `{-1, -1}`.  
Path reconstruction walks `end → ... → start` via parent links, then reverses — **O(L)** where L is path length.

### 8.5 Exploration Trace (`path.h`)
A `Coord cells[MAX_PATH_LEN]` array recording first-discovery order.  
The frontend replays these coordinates in sequence to animate the search.

---

## 9. BFS Algorithm

**File:** `backend/bfs.c`

```
Initialise visited[rows][cols] = false
Initialise parent grid with sentinels
Enqueue start; mark start as visited; add start to trace

While queue is not empty:
    cur = dequeue()
    For each neighbour nb in {Up, Right, Down, Left}:
        If nb is in-bounds AND not wall AND not visited:
            Mark nb as visited
            Set parent[nb] = cur
            Add nb to trace
            If nb == end: STOP (path found)
            Enqueue nb

If end reached:
    Reconstruct path by following parent links from end to start, then reverse
```

**Neighbour order** (deterministic): Up `(-1,0)`, Right `(0,+1)`, Down `(+1,0)`, Left `(0,-1)`.

BFS uses a FIFO queue, which ensures all cells at distance *d* from the start are explored before any cell at distance *d+1*. This guarantees the **shortest path** on an unweighted grid.

---

## 10. DFS Algorithm

**File:** `backend/dfs.c`

```
Initialise visited[rows][cols] = false
Initialise parent grid with sentinels
Push start onto stack

While stack is not empty:
    cur = pop()
    If cur already visited: skip (a cell may be pushed multiple times)
    Mark cur as visited; add cur to trace
    If cur == end: STOP (path found)
    For each neighbour nb in push-order {Left, Down, Right, Up}:
        (Push-order is reversed so Up is processed first on next pop)
        If nb is in-bounds AND not wall AND not visited:
            If parent[nb] not yet set: set parent[nb] = cur
            Push nb

If end reached:
    Reconstruct path via parent links
```

DFS explores **depth-first** — it may find a path that is longer than the shortest. It is not required to be optimal.

---

## 11. Queue and Stack Implementations

### Queue (`queue.c`) — Circular array
```c
typedef struct {
    Coord data[QUEUE_CAPACITY];  // ring buffer
    int   head;                  // front pointer
    int   tail;                  // next write position
    int   count;
} Queue;

// Enqueue (O(1))
data[tail] = c;
tail = (tail + 1) % QUEUE_CAPACITY;
count++;

// Dequeue (O(1))
*out = data[head];
head = (head + 1) % QUEUE_CAPACITY;
count--;
```

### Stack (`stack.c`) — Array-backed
```c
typedef struct {
    Coord data[STACK_CAPACITY];
    int   top;   // -1 when empty
} Stack;

// Push (O(1))
data[++top] = c;

// Pop (O(1))
*out = data[top--];
```

Both structures detect full/empty conditions and return -1 on error. No dynamic allocation is used — memory is bounded at compile time.

---

## 12. Parent Tracking and Path Reconstruction

When a cell `nb` is first discovered from `cur`, we record:
```c
pg.cell[nb.row][nb.col] = cur;
```

To reconstruct the path:
1. Start at `end`
2. Follow parent links: `end → parent[end] → parent[parent[end]] → ... → start`
3. Collect these in reverse order (end-to-start)
4. Reverse the list to get `start → ... → end`

This is **O(L)** where L is the path length.

The start cell's parent is never set (it stays as the sentinel `{-1,-1}`). Path reconstruction stops when it reaches a cell whose parent is the sentinel *and* that cell equals the start.

**Guarantee:** Every consecutive pair in the result is orthogonally adjacent, every cell is traversable, the first cell is start, the last is end.

---

## 13. API Routes

### `GET /api/health`
```json
{ "status": "ok", "app": "MazeLab", "version": "1.0.0" }
```

### `POST /api/solve`
Request:
```json
{
  "algorithm": "bfs",
  "rows": 15, "cols": 21,
  "grid": [[0,0,1,...], ...],
  "start": {"row": 0, "col": 0},
  "end":   {"row": 14, "col": 20}
}
```
`0` = empty, `1` = wall. Valid algorithms: `"bfs"`, `"dfs"`.

Response:
```json
{
  "algorithm":      "bfs",
  "found":          true,
  "visitedCount":   42,
  "pathCellCount":  12,
  "pathLength":     11,
  "executionTimeUs": 38,
  "trace": [{"row":0,"col":0}, ...],
  "path":  [{"row":0,"col":0}, ..., {"row":14,"col":20}]
}
```
- `pathLength` = number of moves = `pathCellCount - 1`
- `trace` = exploration order (first discovery); the frontend animates this
- `path` = final route from start to end; empty array if no path

### `POST /api/maze/generate`
Request: `{ "rows": 15, "cols": 21 }`  
Response: full grid, start, end, seed (same format as solve request body).  
**Algorithm:** randomized depth-first carving (recursive backtracker). Guarantees a solvable maze.

---

## 14. Installation Prerequisites

| Requirement | Windows | Linux/macOS |
|-------------|---------|-------------|
| C compiler | GCC via MinGW (`C:\MinGW\bin\gcc.exe`) | `gcc` or `clang` |
| Make | `mingw32-make` (included in MinGW) | `make` |
| Browser | Any modern browser | Any modern browser |
| Internet | Not required after dependencies are downloaded | Same |

The third-party libraries (Mongoose and cJSON) are already **vendored** in `third_party/`. No internet access is needed to build or run the project.

---

## 15. Run Instructions

### Windows (one command)
```bat
cd e:\DS-Microproject
run.bat
```
Then open **http://127.0.0.1:8080** in your browser.

### Windows (manual)
```bat
cd e:\DS-Microproject
mingw32-make all
mazelab.exe
```

### Custom port
```bat
mazelab.exe 9090
REM Then open http://127.0.0.1:9090
```

### Linux / macOS
```bash
cd /path/to/DS-Microproject
chmod +x run.sh
./run.sh
```

---

## 16. Build and Test Commands

```bat
REM Build the server
mingw32-make all

REM Build and run C unit tests
mingw32-make test

REM Build only (no run) test binary
mingw32-make test_algorithms.exe

REM Run API integration tests (server must be running first)
powershell -ExecutionPolicy Bypass -File tests\test_api.ps1

REM Clean binaries
mingw32-make clean
```

---

## 17. Expected Behaviour

| Scenario | Expected Result |
|----------|----------------|
| Open maze, click Run (BFS) | Exploration animates cell-by-cell; shortest path highlighted in orange |
| Open maze, click Run (DFS) | DFS exploration shown; a valid (not necessarily shortest) path highlighted |
| Destination is walled off | `found: false`; "No path found" status; empty path |
| Start == End | Instant result; path has 1 cell, 0 moves |
| Click "BFS vs DFS" | Both algorithms run; comparison table shows visited cells, path length, timing |
| Generate | Server returns a new random solvable maze |
| Backend is stopped | Status dot turns red; error message shown; no fallback JS pathfinding |
| Edit grid during animation | Animation resets before edit takes effect |

---

## 18. Complexity Analysis

| Component | Time Complexity | Space Complexity |
|-----------|----------------|-----------------|
| Queue enqueue / dequeue | O(1) | O(RC) — pre-allocated |
| Stack push / pop | O(1) | O(RC) — pre-allocated |
| BFS traversal | O(V + E) = O(RC) | O(RC) |
| DFS traversal | O(V + E) = O(RC) | O(RC) |
| Path reconstruction | O(L) | O(L) |
| Grid storage | O(RC) | O(RC) |
| JSON serialization of trace | O(RC) | O(RC) |

**V** = number of cells = R × C.  
**E** = number of edges. Each cell in a 4-neighbor grid has at most 4 edges, so E ≤ 4V, meaning E = O(V) = O(RC).  
Therefore BFS and DFS are both **O(RC)** — linear in the grid size.

**pathLength** = number of moves between consecutive cells = `pathCellCount - 1`.  
**pathCellCount** = number of cells in the path (including start and end).

---

## 19. Limitations

| Limitation | Details |
|-----------|---------|
| No sanitizer support | MinGW GCC 6.3 does not support `-fsanitize=address`. Memory safety verified by code review and tests only. |
| No Valgrind | Not available on Windows natively. |
| No browser automation | No Selenium/Playwright installed; end-to-end tests are manual. |
| Max maze size | 51 × 51 (bounded by pre-allocated arrays to avoid dynamic allocation). |
| Min maze size | 3 × 3 |
| Single-threaded server | Mongoose event loop is single-threaded. Concurrent requests are handled sequentially. |
| No weighted terrain | BFS is optimal only for equal movement costs. Weighted pathfinding (Dijkstra, A\*) is out of scope. |
| Timing precision | Execution times may be 0 µs on fast hardware. `QueryPerformanceCounter` is used on Windows; resolution depends on the system timer. |

---

## 20. Third-Party Attribution

| Library | Version | License | Source |
|---------|---------|---------|--------|
| Mongoose | 7.14 | GNU GPL v2 | https://github.com/cesanta/mongoose |
| cJSON | 1.7.18 | MIT | https://github.com/DaveGamble/cJSON |

Full license texts are in `LICENSES.md`.

---

## 21. Troubleshooting

**"Cannot bind to http://127.0.0.1:8080"**  
Another process is using port 8080. Run on a different port:
```bat
mazelab.exe 9090
```

**"gcc not found"**  
Add MinGW to your PATH: `C:\MinGW\bin`. Or reinstall MinGW.

**Browser shows "Disconnected"**  
The server is not running. Start it with `run.bat` or `mazelab.exe`.

**"Build successful" but no .exe**  
Check that you ran `mingw32-make all` from the project root directory, not from `backend/`.

**Maze does not animate**  
Make sure you pressed **Run** (not just selecting an algorithm). The animation plays automatically after the server responds.

**Generated maze looks different each time**  
This is expected — generation uses a time-based random seed. Pass a fixed `"seed"` in the API body for reproducible mazes.

---

## 22. Viva Preparation

### Why use a queue for BFS?
BFS must explore all cells at distance *d* before any cell at distance *d+1*. A queue (FIFO) enforces this: the first cell enqueued is the first dequeued. If you used a stack, you'd get DFS behaviour, not BFS.

### Why use a stack for DFS?
DFS follows one branch as far as possible before backtracking. A stack (LIFO) naturally achieves this: the most recently pushed neighbour is explored next, driving depth-first exploration.

### Why does BFS find the shortest path on an unweighted grid?
When every move costs the same (1 step), BFS expands "rings" of cells at increasing distances. The first time it reaches the destination, it must have come via the fewest steps, because all shorter distances were already fully explored.

### Why does DFS not guarantee the shortest path?
DFS explores one direction deeply before considering alternatives. The first path it finds may take many unnecessary detours. It is correct (the path is valid), but not necessarily optimal.

### What is a parent array?
A 2-D array where `parent[r][c]` stores the coordinate of the cell that first discovered cell `(r, c)`. After the search, you reconstruct the path by starting at the destination and following parent links back to the start, then reversing.

### What happens when the destination is unreachable?
The queue (or stack) eventually empties without finding the destination. The algorithm returns `found = false`, an empty path, and a trace of all cells that were actually reached.

### Why is C used for the backend?
C gives direct control over data structures, memory layout, and timing. It illustrates exactly how queues, stacks, and arrays work, without abstraction layers. It also makes the algorithm code readable for a college student.

### What responsibilities belong to JavaScript?
JavaScript handles user interaction (drawing walls, choosing algorithms), sends requests to the C API, and *replays* the exploration trace returned by C. It never computes paths itself.

### How do the frontend and backend communicate?
The browser sends HTTP POST requests with JSON bodies containing the maze grid and algorithm choice. The C server parses the JSON, runs the algorithm, and returns a JSON response with the trace, path, and statistics.

### How do you verify that the returned path is valid?
Check four properties:
1. `path[0]` equals the start coordinate
2. `path[last]` equals the end coordinate
3. Every `path[i]` and `path[i+1]` are orthogonally adjacent (differ by exactly one step)
4. No `path[i]` is a wall cell

These are all checked in the C unit tests (`tests/test_algorithms.c`).

---

*MazeLab — Built for clarity, not complexity.*
