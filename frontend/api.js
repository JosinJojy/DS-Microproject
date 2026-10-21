/**
 * api.js — HTTP communication with the MazeLab C backend.
 *
 * All requests use the Fetch API. No external libraries.
 * Request IDs are used to detect and discard stale responses.
 */

'use strict';

const API = (() => {
  const BASE = '';  // same origin — C server serves both API and frontend
  let _requestId = 0;

  /** Internal fetch wrapper with timeout and JSON parsing. */
  async function _fetch(url, options = {}) {
    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 15000); // 15s timeout
    try {
      const resp = await fetch(BASE + url, {
        ...options,
        signal: controller.signal,
        headers: {
          'Content-Type': 'application/json',
          ...(options.headers || {}),
        },
      });
      clearTimeout(timer);
      const text = await resp.text();
      let json;
      try { json = JSON.parse(text); } catch { json = { error: text || 'Empty response' }; }
      return { ok: resp.ok, status: resp.status, data: json };
    } catch (err) {
      clearTimeout(timer);
      if (err.name === 'AbortError') {
        return { ok: false, status: 0, data: { error: 'Request timed out' } };
      }
      return { ok: false, status: 0, data: { error: 'Network error: ' + err.message } };
    }
  }

  /**
   * GET /api/health
   * Returns { status, app, version } or throws.
   */
  async function health() {
    return _fetch('/api/health');
  }

  /**
   * POST /api/solve
   * @param {string}   algorithm  'bfs' | 'dfs'
   * @param {number}   rows
   * @param {number}   cols
   * @param {number[][]} grid     2-D array of 0/1 values
   * @param {{row,col}} start
   * @param {{row,col}} end
   * @returns Promise<{ok, status, data, requestId}>
   */
  function solve(algorithm, rows, cols, grid, start, end) {
    const id = ++_requestId;
    const p = _fetch('/api/solve', {
      method: 'POST',
      body: JSON.stringify({ algorithm, rows, cols, grid, start, end }),
    }).then(result => ({ ...result, requestId: id }));
    return { promise: p, requestId: id };
  }

  /**
   * POST /api/maze/generate
   * @param {number} rows
   * @param {number} cols
   * @returns Promise<{ok, status, data}>
   */
  async function generate(rows, cols) {
    return _fetch('/api/maze/generate', {
      method: 'POST',
      body: JSON.stringify({ rows, cols }),
    });
  }

  return { health, solve, generate };
})();
