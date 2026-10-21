/**
 * app.js — Application state, event coordination, and UI updates.
 *
 * Coordinates: Renderer (grid display), Visualizer (animation),
 * and API (backend communication).
 *
 * All algorithm computation happens in the C backend.
 * This module only manages UI state and delegates work.
 */

'use strict';

/* ── Application state ────────────────────────────────────────── */
const App = (() => {

  let _algo      = 'bfs';       // selected algorithm
  let _editMode  = 'wall';      // 'wall' | 'start' | 'end'
  let _connected = false;       // backend connection status
  let _solving   = false;       // a solve request is in flight
  let _lastRequestId = null;    // for stale-response detection

  /* ── DOM references ───────────────────────────────────────────── */
  const $ = id => document.getElementById(id);
  const els = {
    statusDot:       $('status-dot'),
    statusText:      $('status-text'),
    mazeGrid:        $('maze-grid'),
    btnNewMaze:      $('btn-new-maze'),
    btnGenerate:     $('btn-generate'),
    btnClear:        $('btn-clear'),
    inputRows:       $('input-rows'),
    inputCols:       $('input-cols'),
    btnModeWall:     $('btn-mode-wall'),
    btnModeStart:    $('btn-mode-start'),
    btnModeEnd:      $('btn-mode-end'),
    algoBfs:         $('algo-bfs'),
    algoDfs:         $('algo-dfs'),
    btnRun:          $('btn-run'),
    btnPause:        $('btn-pause'),
    btnStep:         $('btn-step'),
    btnResetViz:     $('btn-reset-viz'),
    speedRange:      $('speed-range'),
    speedValue:      $('speed-value'),
    btnCompare:      $('btn-compare'),
    compareResult:   $('compare-result'),
    statVisited:     $('stat-visited'),
    statPathCells:   $('stat-path-cells'),
    statPathLen:     $('stat-path-len'),
    statTime:        $('stat-time'),
    statResult:      $('stat-result'),
    expAlgoName:     $('exp-algo-name'),
    expDsBadge:      $('exp-ds-badge'),
    expDescription:  $('exp-description'),
    expStatusValue:  $('exp-status-value'),
    expStep:         $('exp-step'),
    errorBanner:     $('error-banner'),
    errorText:       $('error-text'),
    errorClose:      $('error-close'),
  };

  /* ── Algorithm metadata ─────────────────────────────────────── */
  const ALGO_META = {
    bfs: {
      name:  'BFS',
      ds:    'Queue (FIFO)',
      desc:  'Breadth-First Search explores cells layer by layer, using a queue (FIFO). ' +
             'Every cell at distance d is visited before any cell at distance d+1. ' +
             'On an unweighted grid, BFS always finds the shortest path when one exists.',
    },
    dfs: {
      name:  'DFS',
      ds:    'Stack (LIFO)',
      desc:  'Depth-First Search explores as deep as possible along each branch before ' +
             'backtracking, using a stack (LIFO). DFS may find a longer path than the ' +
             'shortest possible route — it is not guaranteed to be optimal.',
    },
  };

  /* ── Initialise ──────────────────────────────────────────────── */
  function init() {
    // Initialise sub-modules
    Renderer.init(els.mazeGrid, _onCellEdit);
    Visualizer.init(_onVisualizerStep, _onVisualizerDone);

    // Build default maze
    _buildDefaultMaze();

    // Wire events
    els.btnNewMaze.addEventListener('click',   () => _generateMaze());
    els.btnGenerate.addEventListener('click',  () => _generateMaze());
    els.btnClear.addEventListener('click',     () => _clearMaze());
    els.btnRun.addEventListener('click',       () => _runAlgorithm());
    els.btnPause.addEventListener('click',     () => _togglePause());
    els.btnStep.addEventListener('click',      () => _stepForward());
    els.btnResetViz.addEventListener('click',  () => _resetVisualization());
    els.btnCompare.addEventListener('click',   () => _runComparison());
    els.errorClose.addEventListener('click',   () => _hideError());

    // Edit mode buttons
    [els.btnModeWall, els.btnModeStart, els.btnModeEnd].forEach(btn => {
      btn.addEventListener('click', () => _setEditMode(btn.dataset.mode));
    });

    // Algorithm selector
    document.querySelectorAll('input[name="algorithm"]').forEach(radio => {
      radio.addEventListener('change', () => {
        _algo = radio.value;
        _updateAlgoInfo();
      });
    });

    // Dimension inputs
    els.inputRows.addEventListener('change', () => _onDimChange());
    els.inputCols.addEventListener('change', () => _onDimChange());

    // Speed slider
    els.speedRange.addEventListener('input', () => {
      const ms = parseInt(els.speedRange.value, 10);
      els.speedValue.textContent = ms + 'ms';
      Visualizer.setSpeed(ms);
    });

    // Keyboard shortcut: Space = run/pause when not in an input
    document.addEventListener('keydown', e => {
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'BUTTON') return;
      if (e.code === 'Space') { e.preventDefault(); _toggleRunPause(); }
      if (e.code === 'KeyR')  { e.preventDefault(); _resetVisualization(); }
      if (e.code === 'Period'){ e.preventDefault(); _stepForward(); }
    });

    // Initial UI state
    _updateAlgoInfo();
    _updatePlaybackButtons();
    _setStatus('connecting');

    // Check backend health
    _checkHealth();
    setInterval(_checkHealth, 10000); // poll every 10 s
  }

  /* ── Backend health check ────────────────────────────────────── */
  async function _checkHealth() {
    try {
      const r = await API.health();
      if (r.ok && r.data.status === 'ok') {
        _setStatus('ok', 'Connected — ' + r.data.app + ' ' + r.data.version);
        if (!_connected) {
          _connected = true;
          _enableControls(true);
        }
      } else {
        _setStatus('err', 'Server error');
        _connected = false;
        _enableControls(false);
      }
    } catch {
      _setStatus('err', 'Disconnected');
      _connected = false;
      _enableControls(false);
    }
  }

  function _setStatus(state, text) {
    els.statusDot.className = 'status-dot ' + (state === 'ok' ? 'ok' : state === 'err' ? 'err' : 'warn');
    els.statusText.textContent = text || (state === 'ok' ? 'Connected' : state === 'err' ? 'Disconnected' : 'Connecting…');
  }

  function _enableControls(enabled) {
    [els.btnRun, els.btnGenerate, els.btnNewMaze, els.btnCompare].forEach(b => {
      b.disabled = !enabled;
    });
  }

  /* ── Default maze ────────────────────────────────────────────── */
  function _buildDefaultMaze() {
    const rows = parseInt(els.inputRows.value, 10) || 15;
    const cols = parseInt(els.inputCols.value, 10) || 21;
    const grid = [];
    for (let r = 0; r < rows; r++) {
      grid[r] = new Array(cols).fill(0);
    }
    const start = { row: 1, col: 1 };
    const end   = { row: rows - 2, col: cols - 2 };
    Renderer.buildGrid(rows, cols, grid, start, end);
  }

  /* ── Dimension change ────────────────────────────────────────── */
  function _onDimChange() {
    let rows = parseInt(els.inputRows.value, 10);
    let cols = parseInt(els.inputCols.value, 10);
    rows = Math.max(3, Math.min(51, rows || 15));
    cols = Math.max(3, Math.min(51, cols || 21));
    els.inputRows.value = rows;
    els.inputCols.value = cols;
    _resetVisualization();
    _buildDefaultMaze();
  }

  /* ── Cell edit callback (from Renderer) ───────────────────────── */
  function _onCellEdit(row, col, mode, dragMode) {
    // Block editing while a solve is running
    if (_solving) return;
    // Also block while playing/paused (maze changed would invalidate result)
    if (Visualizer.getState() === 'playing' || Visualizer.getState() === 'paused') {
      _resetVisualization();
    }

    if (mode === 'wall') {
      const isWall = dragMode === 'draw';
      // Don't wall over start or end
      const s = Renderer.getStart();
      const e = Renderer.getEnd();
      if ((row === s.row && col === s.col) || (row === e.row && col === e.col)) return;
      Renderer.setWall(row, col, isWall);

    } else if (mode === 'start') {
      // Can't place start on a wall
      if (Renderer.getGrid()[row][col] === 1) return;
      // Can't place on end
      const e = Renderer.getEnd();
      if (row === e.row && col === e.col) return;
      Renderer.setStart(row, col);

    } else if (mode === 'end') {
      // Can't place end on a wall
      if (Renderer.getGrid()[row][col] === 1) return;
      // Can't place on start
      const s = Renderer.getStart();
      if (row === s.row && col === s.col) return;
      Renderer.setEnd(row, col);
    }
  }

  /* ── Edit mode toggle ────────────────────────────────────────── */
  function _setEditMode(mode) {
    _editMode = mode;
    Renderer.setEditMode(mode);

    [els.btnModeWall, els.btnModeStart, els.btnModeEnd].forEach(btn => {
      const active = btn.dataset.mode === mode;
      btn.classList.toggle('active', active);
      btn.setAttribute('aria-pressed', String(active));
    });
  }

  /* ── Generate maze via C API ────────────────────────────────── */
  async function _generateMaze() {
    if (!_connected) { _showError('Server not connected.'); return; }
    _resetVisualization();
    _hideError();

    const rows = parseInt(els.inputRows.value, 10);
    const cols = parseInt(els.inputCols.value, 10);

    _setExpStatus('Generating…');
    els.btnGenerate.disabled = true;

    try {
      const r = await API.generate(rows, cols);
      if (!r.ok) {
        _showError(r.data.error || 'Generation failed');
        return;
      }
      const d = r.data;
      Renderer.buildGrid(d.rows, d.cols, d.grid, d.start, d.end);
      // Sync dimension inputs with actual generated size (may differ if odd enforcement applied)
      els.inputRows.value = d.rows;
      els.inputCols.value = d.cols;
      _setExpStatus('Ready');
    } catch (e) {
      _showError('Generation failed: ' + e.message);
    } finally {
      els.btnGenerate.disabled = false;
    }
  }

  /* ── Clear maze ─────────────────────────────────────────────── */
  function _clearMaze() {
    _resetVisualization();
    const rows = Renderer.getRows();
    const cols = Renderer.getCols();
    const grid = [];
    for (let r = 0; r < rows; r++) grid[r] = new Array(cols).fill(0);
    const start = Renderer.getStart();
    const end   = Renderer.getEnd();
    Renderer.buildGrid(rows, cols, grid, start, end);
    _setExpStatus('Ready');
    _clearStats();
  }

  /* ── Run algorithm ───────────────────────────────────────────── */
  async function _runAlgorithm() {
    if (!_connected)  { _showError('Server not connected.'); return; }
    if (_solving)     return;

    // If already animated, reset first
    Visualizer.reset();
    Renderer.clearOverlay();
    Renderer.setRunning(true);
    _hideError();

    _solving = true;
    _setExpStatus('Running…');
    _updatePlaybackButtons();
    _clearStats();

    const grid  = Renderer.getGrid();
    const start = Renderer.getStart();
    const end   = Renderer.getEnd();
    const rows  = Renderer.getRows();
    const cols  = Renderer.getCols();

    const { promise, requestId } = API.solve(_algo, rows, cols, grid, start, end);
    _lastRequestId = requestId;

    let result;
    try {
      result = await promise;
    } catch (e) {
      if (requestId !== _lastRequestId) return; // stale, ignore
      _showError('Network error: ' + e.message);
      _solving = false;
      Renderer.setRunning(false);
      _setExpStatus('Error');
      _updatePlaybackButtons();
      return;
    }

    // Stale response check
    if (requestId !== _lastRequestId) return;

    _solving = false;

    if (!result.ok) {
      _showError(result.data.error || 'Solve request failed');
      Renderer.setRunning(false);
      _setExpStatus('Error');
      _updatePlaybackButtons();
      return;
    }

    const d = result.data;

    // Update statistics from actual backend data
    _updateStats(d);

    // Load trace into visualizer and start playing
    Visualizer.load(d.trace || [], d.path || [], d.found);
    Visualizer.play();
    _setExpStatus('Animating…');
    _updatePlaybackButtons();
  }

  /* ── Pause / resume ─────────────────────────────────────────── */
  function _togglePause() {
    const state = Visualizer.getState();
    if (state === 'playing') {
      Visualizer.pause();
      _setExpStatus('Paused');
    } else if (state === 'paused') {
      Visualizer.play();
      _setExpStatus('Animating…');
    }
    _updatePlaybackButtons();
  }

  function _toggleRunPause() {
    const state = Visualizer.getState();
    if (state === 'idle' || state === 'done') {
      _runAlgorithm();
    } else {
      _togglePause();
    }
  }

  /* ── Step forward ───────────────────────────────────────────── */
  function _stepForward() {
    Visualizer.step();
    _setExpStatus('Stepping');
    _updatePlaybackButtons();
  }

  /* ── Reset visualization ────────────────────────────────────── */
  function _resetVisualization() {
    Visualizer.reset();
    Renderer.clearOverlay();
    Renderer.setRunning(false);
    _clearStats();
    _setExpStatus('Ready');
    _updatePlaybackButtons();
    els.compareResult.hidden = true;
  }

  /* ── Algorithm comparison ───────────────────────────────────── */
  async function _runComparison() {
    if (!_connected) { _showError('Server not connected.'); return; }
    if (_solving)     return;

    _resetVisualization();
    _hideError();
    els.btnCompare.disabled = true;
    _setExpStatus('Comparing…');

    const grid  = Renderer.getGrid();
    const start = Renderer.getStart();
    const end   = Renderer.getEnd();
    const rows  = Renderer.getRows();
    const cols  = Renderer.getCols();

    // Fire both requests simultaneously with the same maze data
    const bfsCall = API.solve('bfs', rows, cols, grid, start, end);
    const dfsCall = API.solve('dfs', rows, cols, grid, start, end);

    let bfsRes, dfsRes;
    try {
      [bfsRes, dfsRes] = await Promise.all([bfsCall.promise, dfsCall.promise]);
    } catch (e) {
      _showError('Comparison failed: ' + e.message);
      els.btnCompare.disabled = false;
      _setExpStatus('Error');
      return;
    }

    els.btnCompare.disabled = false;

    if (!bfsRes.ok || !dfsRes.ok) {
      const msg = (!bfsRes.ok ? bfsRes.data.error : dfsRes.data.error) || 'Request failed';
      _showError('Comparison error: ' + msg);
      _setExpStatus('Error');
      return;
    }

    const b = bfsRes.data;
    const d = dfsRes.data;

    // Show comparison table
    _renderCompare(b, d);
    _setExpStatus('Comparison done');

    // Animate the BFS result (more instructive for shortest path)
    Visualizer.load(b.trace || [], b.path || [], b.found);
    Visualizer.play();
    _updatePlaybackButtons();
  }

  function _renderCompare(b, d) {
    const bVis  = b.visitedCount  || 0;
    const dVis  = d.visitedCount  || 0;
    const bPath = b.pathLength    || 0;
    const dPath = d.pathLength    || 0;
    const bTime = b.executionTimeUs || 0;
    const dTime = d.executionTimeUs || 0;

    // Which has shorter path?
    const bFaster = bPath <= dPath && b.found;
    const dFaster = dPath < bPath && d.found;

    const table = `
      <table class="compare-table">
        <thead>
          <tr>
            <th>Metric</th>
            <th>BFS</th>
            <th>DFS</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td>Found path</td>
            <td>${b.found ? '✓ Yes' : '✗ No'}</td>
            <td>${d.found ? '✓ Yes' : '✗ No'}</td>
          </tr>
          <tr>
            <td>Visited cells</td>
            <td>${bVis}</td>
            <td>${dVis}</td>
          </tr>
          <tr>
            <td>Path length (moves)</td>
            <td class="${bFaster ? 'compare-highlight' : ''}">${b.found ? bPath : '—'}</td>
            <td class="${dFaster ? 'compare-highlight' : ''}">${d.found ? dPath : '—'}</td>
          </tr>
          <tr>
            <td>Server time (µs)</td>
            <td>${bTime}</td>
            <td>${dTime}</td>
          </tr>
        </tbody>
      </table>
      <p class="compare-note">
        BFS guarantees the shortest path on an unweighted grid. DFS does not.
        Execution times are measured by the C server and may be very small (sub-microsecond);
        interpret them as indicative, not precise benchmarks.
      </p>`;

    els.compareResult.innerHTML = table;
    els.compareResult.hidden = false;
  }

  /* ── Visualizer callbacks ────────────────────────────────────── */
  function _onVisualizerStep(phase, index, total, coord) {
    if (phase === 'trace') {
      const msg = _algo === 'bfs'
        ? `BFS discovers cell (${coord.row}, ${coord.col}) and adds it to the queue. ` +
          `Exploring cell ${index + 1} of ${total}.`
        : `DFS pops (${coord.row}, ${coord.col}) from the stack and explores it. ` +
          `Step ${index + 1} of ${total}.`;
      els.expStep.textContent = msg;
    } else {
      const msg = `Path reconstruction: following parent links to recover the route. ` +
                  `Cell ${index + 1} of ${total}: (${coord.row}, ${coord.col}).`;
      els.expStep.textContent = msg;
    }
    _updatePlaybackButtons();
  }

  function _onVisualizerDone(found) {
    _setExpStatus(found ? 'Path found ✓' : 'No path found ✗');
    const msg = found
      ? 'The algorithm has finished. The highlighted path shows the route ' +
        'from start to destination as reconstructed from the parent array.'
      : 'The algorithm exhausted all reachable cells without finding the destination. ' +
        'The destination is unreachable from the start cell.';
    els.expStep.textContent = msg;
    _updatePlaybackButtons();
  }

  /* ── Stats display ──────────────────────────────────────────── */
  function _updateStats(d) {
    els.statVisited.textContent    = d.visitedCount    ?? '—';
    els.statPathCells.textContent  = d.pathCellCount   ?? '—';
    els.statPathLen.textContent    = d.pathLength      ?? '—';
    const us = d.executionTimeUs;
    els.statTime.textContent       = (us !== undefined) ? us + ' µs' : '—';
    els.statResult.textContent     = d.found ? 'Path found' : 'No path';
    els.statResult.className       = 'stat-value ' + (d.found ? 'found' : 'notfound');
  }

  function _clearStats() {
    [els.statVisited, els.statPathCells, els.statPathLen, els.statTime].forEach(el => {
      el.textContent = '—';
      el.className = 'stat-value';
    });
    els.statResult.textContent = '—';
    els.statResult.className = 'stat-value';
  }

  /* ── Explanation / status ────────────────────────────────────── */
  function _updateAlgoInfo() {
    const meta = ALGO_META[_algo];
    els.expAlgoName.textContent    = meta.name;
    els.expDsBadge.textContent     = meta.ds;
    els.expDescription.textContent = meta.desc;
  }

  function _setExpStatus(text) {
    els.expStatusValue.textContent = text;
  }

  /* ── Error display ──────────────────────────────────────────── */
  function _showError(msg) {
    els.errorText.textContent = msg;
    els.errorBanner.hidden = false;
  }

  function _hideError() {
    els.errorBanner.hidden = true;
  }

  /* ── Playback button state ───────────────────────────────────── */
  function _updatePlaybackButtons() {
    const state = Visualizer.getState();
    const idle  = state === 'idle' || state === 'done';

    els.btnRun.disabled     = _solving;
    els.btnPause.disabled   = idle || _solving;
    els.btnStep.disabled    = (state === 'idle' || state === 'done') || _solving;
    els.btnResetViz.disabled = false;
    els.btnCompare.disabled  = _solving;

    // Update pause button label
    if (state === 'playing') {
      els.btnPause.querySelector('svg').innerHTML = '<rect x="3" y="2" width="4" height="12"/><rect x="9" y="2" width="4" height="12"/>';
      els.btnPause.childNodes[1].textContent = ' Pause';
    } else {
      els.btnPause.querySelector('svg').innerHTML = '<polygon points="3,2 13,8 3,14"/>';
      els.btnPause.childNodes[1] && (els.btnPause.childNodes[1].textContent = ' Resume');
    }
  }

  return { init };
})();

/* ── Bootstrap ─────────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => App.init());
