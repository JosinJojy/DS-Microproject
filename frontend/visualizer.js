/**
 * visualizer.js — Animation playback engine.
 *
 * Consumes the actual trace and path returned by the C backend.
 * Never computes pathfinding itself — only replays the server result.
 *
 * States:
 *   'idle'    — no animation loaded
 *   'playing' — automatically advancing through trace/path
 *   'paused'  — user paused; step() still works
 *   'done'    — all frames consumed
 */

'use strict';

const Visualizer = (() => {
  // Animation state
  let _state    = 'idle'; // 'idle' | 'playing' | 'paused' | 'done'
  let _trace    = [];     // array of {row, col} — exploration order from backend
  let _path     = [];     // array of {row, col} — final path from backend
  let _traceIdx = 0;      // current position in trace
  let _pathIdx  = 0;      // current position in path (after trace done)
  let _phase    = 'trace'; // 'trace' | 'path'
  let _timerId  = null;   // setTimeout id for auto-play

  let _intervalMs = 50;   // animation step duration (ms)

  // Callbacks
  let _onStep   = null; // (phase, index, total, coord) => void
  let _onDone   = null; // (found) => void
  let _found    = false;

  /* ── Initialise ─────────────────────────────────────────────── */
  function init(onStep, onDone) {
    _onStep = onStep;
    _onDone = onDone;
  }

  /* ── Load a new result from the backend ─────────────────────── */
  function load(trace, path, found) {
    _cancelTimer();
    _trace    = trace  || [];
    _path     = path   || [];
    _found    = found  || false;
    _traceIdx = 0;
    _pathIdx  = 0;
    _phase    = 'trace';
    _state    = 'paused'; // loaded but not yet playing
    Renderer.clearOverlay();
  }

  /* ── Play ───────────────────────────────────────────────────── */
  function play() {
    if (_state === 'done' || _state === 'idle') return;
    _state = 'playing';
    _scheduleNext();
  }

  /* ── Pause ──────────────────────────────────────────────────── */
  function pause() {
    if (_state !== 'playing') return;
    _cancelTimer();
    _state = 'paused';
  }

  /* ── Step forward one frame ─────────────────────────────────── */
  function step() {
    if (_state === 'idle' || _state === 'done') return;
    _cancelTimer();
    _advance();
    if (_state === 'playing') {
      _state = 'paused'; // stepping always pauses
    }
  }

  /* ── Reset ──────────────────────────────────────────────────── */
  function reset() {
    _cancelTimer();
    _trace    = [];
    _path     = [];
    _traceIdx = 0;
    _pathIdx  = 0;
    _phase    = 'trace';
    _state    = 'idle';
    Renderer.clearOverlay();
    Renderer.setRunning(false);
  }

  /* ── Set animation speed ────────────────────────────────────── */
  function setSpeed(ms) {
    _intervalMs = Math.max(10, Math.min(2000, ms));
  }

  /* ── Get current state ──────────────────────────────────────── */
  function getState() { return _state; }

  /* ── Internal: schedule next frame ─────────────────────────── */
  function _scheduleNext() {
    _cancelTimer();
    _timerId = setTimeout(_onTick, _intervalMs);
  }

  function _onTick() {
    _timerId = null;
    _advance();
    if (_state === 'playing') {
      _scheduleNext();
    }
  }

  /* ── Internal: advance one animation frame ──────────────────── */
  function _advance() {
    if (_phase === 'trace') {
      if (_traceIdx >= _trace.length) {
        // All exploration shown — switch to path phase
        _phase = 'path';
        _pathIdx = 0;
        _advancePath();
      } else {
        _advanceTrace();
      }
    } else {
      _advancePath();
    }
  }

  function _advanceTrace() {
    if (_traceIdx >= _trace.length) {
      _phase = 'path';
      _pathIdx = 0;
      _advancePath();
      return;
    }

    const coord = _trace[_traceIdx];

    // Remove 'current' highlight from previous step
    if (_traceIdx > 0) {
      const prev = _trace[_traceIdx - 1];
      // Don't overwrite start/end cells with 'visited' visually when they
      // are the same as start or end — Renderer._cellState handles priority,
      // but we set overlay here anyway; the renderer will apply start/end
      // after overlay in practice.  We set 'visited' overlay regardless.
      Renderer.setOverlay(prev.row, prev.col, 'visited');
    }

    // Highlight current cell
    Renderer.setOverlay(coord.row, coord.col, 'current');

    _onStep && _onStep('trace', _traceIdx, _trace.length, coord);
    _traceIdx++;
  }

  function _advancePath() {
    if (!_found || _path.length === 0) {
      _finish();
      return;
    }

    if (_pathIdx >= _path.length) {
      _finish();
      return;
    }

    // On first path step: clear the 'current' from last trace cell
    if (_pathIdx === 0 && _trace.length > 0) {
      const lastTrace = _trace[_trace.length - 1];
      Renderer.setOverlay(lastTrace.row, lastTrace.col, 'visited');
    }

    const coord = _path[_pathIdx];
    Renderer.setOverlay(coord.row, coord.col, 'path');

    _onStep && _onStep('path', _pathIdx, _path.length, coord);
    _pathIdx++;

    if (_pathIdx >= _path.length) {
      _finish();
    }
  }

  function _finish() {
    _cancelTimer();
    _state = 'done';
    Renderer.setRunning(false);
    _onDone && _onDone(_found);
  }

  /* ── Internal: cancel pending timer ────────────────────────── */
  function _cancelTimer() {
    if (_timerId !== null) {
      clearTimeout(_timerId);
      _timerId = null;
    }
  }

  return {
    init,
    load,
    play,
    pause,
    step,
    reset,
    setSpeed,
    getState,
  };
})();
