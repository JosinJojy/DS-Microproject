/**
 * renderer.js — Maze grid rendering and cell-state presentation.
 *
 * Renders the maze as a CSS grid of div.cell elements.
 * Uses event delegation: one mousedown/mousemove/mouseup listener on the
 * grid container rather than per-cell listeners.
 *
 * Cell states:
 *   'empty'   — traversable open cell (default)
 *   'wall'    — impassable wall
 *   'start'   — start coordinate (also open)
 *   'end'     — destination coordinate (also open)
 *   'visited' — explored during algorithm replay
 *   'current' — the cell being processed at the current animation step
 *   'path'    — final route cell
 */

'use strict';

const Renderer = (() => {
  // References set by init()
  let _gridEl = null;
  let _rows = 0;
  let _cols = 0;

  // Logical state (not the visual overlay — those come from Visualizer)
  let _grid = [];   // 2-D array of 0 (empty) or 1 (wall)
  let _start = { row: 0, col: 0 };
  let _end   = { row: 0, col: 0 };

  // Callbacks set by app.js
  let _onCellEdit  = null; // (row, col, mode) => void
  let _onCellClick = null; // (row, col) => void

  // Drag state
  let _dragging    = false;
  let _dragMode    = null;   // 'draw' | 'erase' — set on mousedown
  let _editMode    = 'wall'; // current toolbar mode

  // Overlay state from Visualizer (visual only — not the logical grid)
  let _overlay = [];  // 2-D array of overlay state strings | null

  /* ── Initialise ─────────────────────────────────────────────── */
  function init(gridElement, onCellEdit) {
    _gridEl     = gridElement;
    _onCellEdit = onCellEdit;

    // Pointer events for drag-draw
    _gridEl.addEventListener('mousedown',  _onMouseDown);
    _gridEl.addEventListener('mouseover',  _onMouseOver);
    document.addEventListener('mouseup',   _onMouseUp);

    // Keyboard navigation
    _gridEl.addEventListener('keydown',    _onKeyDown);

    // Touch support
    _gridEl.addEventListener('touchstart', _onTouchStart, { passive: false });
    _gridEl.addEventListener('touchmove',  _onTouchMove,  { passive: false });
    _gridEl.addEventListener('touchend',   _onTouchEnd,   { passive: false });
  }

  /* ── Build a fresh grid ─────────────────────────────────────── */
  function buildGrid(rows, cols, initialGrid, start, end) {
    _rows  = rows;
    _cols  = cols;
    _start = { ...start };
    _end   = { ...end };

    // Clone grid data
    _grid = [];
    for (let r = 0; r < rows; r++) {
      _grid[r] = [];
      for (let c = 0; c < cols; c++) {
        _grid[r][c] = initialGrid ? initialGrid[r][c] : 0;
      }
    }

    // Reset overlay
    _overlay = [];
    for (let r = 0; r < rows; r++) {
      _overlay[r] = new Array(cols).fill(null);
    }

    _render();
  }

  /* ── Full render ────────────────────────────────────────────── */
  function _render() {
    const cellPx = _computeCellSize();

    _gridEl.style.gridTemplateColumns = `repeat(${_cols}, ${cellPx}px)`;
    _gridEl.style.gridTemplateRows    = `repeat(${_rows}, ${cellPx}px)`;

    // Build HTML string for performance
    const frags = [];
    for (let r = 0; r < _rows; r++) {
      for (let c = 0; c < _cols; c++) {
        const state = _cellState(r, c);
        frags.push(
          `<div class="cell" data-r="${r}" data-c="${c}" ` +
          `data-state="${state}" ` +
          `role="gridcell" tabindex="-1" ` +
          `aria-label="Row ${r + 1} Column ${c + 1} ${state}">` +
          `</div>`
        );
      }
    }
    _gridEl.innerHTML = frags.join('');

    // Make first cell tabbable
    const first = _gridEl.querySelector('.cell');
    if (first) first.tabIndex = 0;
  }

  /* ── Determine visual state of a cell ──────────────────────── */
  function _cellState(r, c) {
    // Overlay takes priority over logical state (for animation)
    const ov = _overlay[r] && _overlay[r][c];
    if (ov) return ov;

    // Start/end markers
    if (r === _start.row && c === _start.col) return 'start';
    if (r === _end.row   && c === _end.col)   return 'end';

    // Wall vs empty
    return _grid[r][c] === 1 ? 'wall' : 'empty';
  }

  /* ── Update a single cell's visual state ───────────────────── */
  function _updateCell(r, c) {
    const el = _getCell(r, c);
    if (!el) return;
    const state = _cellState(r, c);
    el.dataset.state = state;
    el.setAttribute('aria-label', `Row ${r + 1} Column ${c + 1} ${state}`);
  }

  /* ── Compute a sensible cell pixel size ─────────────────────── */
  function _computeCellSize() {
    const viewport = _gridEl.parentElement;
    const maxW = (viewport.clientWidth  - 20) || 600;
    const maxH = (viewport.clientHeight - 20) || 400;
    const byW  = Math.floor(maxW / _cols);
    const byH  = Math.floor(maxH / _rows);
    return Math.max(6, Math.min(byW, byH, 32));
  }

  /* ── Element helper ─────────────────────────────────────────── */
  function _getCell(r, c) {
    return _gridEl.querySelector(`[data-r="${r}"][data-c="${c}"]`);
  }

  function _coordFromEl(el) {
    if (!el || !el.classList.contains('cell')) return null;
    return { row: parseInt(el.dataset.r, 10), col: parseInt(el.dataset.c, 10) };
  }

  /* ── Mouse event delegation ─────────────────────────────────── */
  function _onMouseDown(e) {
    if (e.button !== 0) return;
    const coord = _coordFromEl(e.target);
    if (!coord) return;
    e.preventDefault();
    _dragging = true;
    // Determine draw vs erase based on current cell state + mode
    if (_editMode === 'wall') {
      _dragMode = _grid[coord.row][coord.col] === 1 ? 'erase' : 'draw';
    } else {
      _dragMode = _editMode; // 'start' or 'end'
    }
    _onCellEdit && _onCellEdit(coord.row, coord.col, _editMode, _dragMode);
  }

  function _onMouseOver(e) {
    if (!_dragging) return;
    const coord = _coordFromEl(e.target);
    if (!coord) return;
    _onCellEdit && _onCellEdit(coord.row, coord.col, _editMode, _dragMode);
  }

  function _onMouseUp() {
    _dragging = false;
    _dragMode = null;
  }

  /* ── Touch event delegation ─────────────────────────────────── */
  function _getTouchCoord(touch) {
    const el = document.elementFromPoint(touch.clientX, touch.clientY);
    return _coordFromEl(el);
  }

  function _onTouchStart(e) {
    e.preventDefault();
    const coord = _getTouchCoord(e.touches[0]);
    if (!coord) return;
    _dragging = true;
    _dragMode = _editMode === 'wall'
      ? (_grid[coord.row][coord.col] === 1 ? 'erase' : 'draw')
      : _editMode;
    _onCellEdit && _onCellEdit(coord.row, coord.col, _editMode, _dragMode);
  }

  function _onTouchMove(e) {
    e.preventDefault();
    if (!_dragging) return;
    const coord = _getTouchCoord(e.touches[0]);
    if (!coord) return;
    _onCellEdit && _onCellEdit(coord.row, coord.col, _editMode, _dragMode);
  }

  function _onTouchEnd() {
    _dragging = false;
    _dragMode = null;
  }

  /* ── Keyboard navigation within grid ───────────────────────── */
  function _onKeyDown(e) {
    const el = e.target;
    if (!el.classList.contains('cell')) return;

    const r = parseInt(el.dataset.r, 10);
    const c = parseInt(el.dataset.c, 10);

    let nr = r, nc = c;
    switch (e.key) {
      case 'ArrowUp':    nr = r - 1; e.preventDefault(); break;
      case 'ArrowDown':  nr = r + 1; e.preventDefault(); break;
      case 'ArrowLeft':  nc = c - 1; e.preventDefault(); break;
      case 'ArrowRight': nc = c + 1; e.preventDefault(); break;
      case 'Enter':
      case ' ':
        _onCellEdit && _onCellEdit(r, c, _editMode, _editMode === 'wall'
          ? (_grid[r][c] === 1 ? 'erase' : 'draw')
          : _editMode);
        e.preventDefault();
        return;
      default: return;
    }

    if (nr >= 0 && nr < _rows && nc >= 0 && nc < _cols) {
      const target = _getCell(nr, nc);
      if (target) {
        // Move tabindex
        el.tabIndex = -1;
        target.tabIndex = 0;
        target.focus();
      }
    }
  }

  /* ── Public API for app.js to mutate logical state ─────────── */

  /** Set or clear a wall at (r, c). Updates visual immediately. */
  function setWall(r, c, isWall) {
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) return;
    _grid[r][c] = isWall ? 1 : 0;
    _updateCell(r, c);
  }

  /** Move the start cell. */
  function setStart(r, c) {
    const old = { ..._start };
    _start = { row: r, col: c };
    _updateCell(old.row, old.col);
    _updateCell(r, c);
  }

  /** Move the end cell. */
  function setEnd(r, c) {
    const old = { ..._end };
    _end = { row: r, col: c };
    _updateCell(old.row, old.col);
    _updateCell(r, c);
  }

  /** Change the active editing mode. */
  function setEditMode(mode) {
    _editMode = mode;
  }

  /* ── Overlay API for Visualizer ─────────────────────────────── */

  /** Apply an overlay state to a single cell (does not change logical grid). */
  function setOverlay(r, c, state) {
    if (r < 0 || r >= _rows || c < 0 || c >= _cols) return;
    if (!_overlay[r]) return;
    _overlay[r][c] = state;
    _updateCell(r, c);
  }

  /** Clear all overlay states and re-render to logical state. */
  function clearOverlay() {
    for (let r = 0; r < _rows; r++) {
      if (_overlay[r]) _overlay[r].fill(null);
    }
    // Batch update
    _gridEl.querySelectorAll('.cell').forEach(el => {
      const r = parseInt(el.dataset.r, 10);
      const c = parseInt(el.dataset.c, 10);
      el.dataset.state = _cellState(r, c);
    });
  }

  /** Mark the grid as running (blocks editing hover effects). */
  function setRunning(running) {
    _gridEl.classList.toggle('running', running);
  }

  /* ── Getters ────────────────────────────────────────────────── */
  function getGrid()    { return _grid.map(r => [...r]); }
  function getStart()   { return { ..._start }; }
  function getEnd()     { return { ..._end };   }
  function getRows()    { return _rows; }
  function getCols()    { return _cols; }

  return {
    init,
    buildGrid,
    setWall,
    setStart,
    setEnd,
    setEditMode,
    setOverlay,
    clearOverlay,
    setRunning,
    getGrid,
    getStart,
    getEnd,
    getRows,
    getCols,
  };
})();
