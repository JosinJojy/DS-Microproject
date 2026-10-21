#!/usr/bin/env bash
# MazeLab API Integration Tests — Shell script (Linux/macOS)
# Usage: ./tests/test_api.sh [port]
# Requires: curl, a running mazelab server

PORT="${1:-8080}"
BASE="http://127.0.0.1:$PORT"
PASS=0
FAIL=0

GREEN='\033[0;32m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

check() {
    local name="$1"; local cond="$2"
    if [ "$cond" = "0" ]; then
        echo -e "  ${GREEN}PASS${NC}  $name"; PASS=$((PASS+1))
    else
        echo -e "  ${RED}FAIL${NC}  $name"; FAIL=$((FAIL+1))
    fi
}

api_get() {
    curl -s -o /tmp/ml_body.json -w "%{http_code}" "$BASE$1"
}

api_post() {
    curl -s -o /tmp/ml_body.json -w "%{http_code}" \
         -X POST -H "Content-Type: application/json" -d "$2" "$BASE$1"
}

jq_val() { cat /tmp/ml_body.json | python3 -c "import sys,json; d=json.load(sys.stdin); print(d$1)" 2>/dev/null; }

echo ""
echo -e "${CYAN}==========================================${NC}"
echo -e "${CYAN}  MazeLab API Integration Tests (shell)${NC}"
echo -e "${CYAN}  Target: $BASE${NC}"
echo -e "${CYAN}==========================================${NC}"

# --- Health ---
echo ""
echo "--- Health endpoint ---"
CODE=$(api_get /api/health)
check "GET /api/health returns 200" $([ "$CODE" = "200" ]; echo $?)
check "health status=ok" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if d.get('status')=='ok' else 1)" 2>/dev/null; echo $?)
check "health app=MazeLab" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if d.get('app')=='MazeLab' else 1)" 2>/dev/null; echo $?)

# --- BFS solve ---
echo ""
echo "--- Solve: BFS ---"
BFS_BODY='{"algorithm":"bfs","rows":5,"cols":5,"grid":[[0,0,0,0,0],[0,1,1,1,0],[0,0,0,1,0],[0,1,0,1,0],[0,1,0,0,0]],"start":{"row":0,"col":0},"end":{"row":4,"col":4}}'
CODE=$(api_post /api/solve "$BFS_BODY")
check "POST /api/solve BFS returns 200" $([ "$CODE" = "200" ]; echo $?)
check "BFS found=true" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if d.get('found') else 1)" 2>/dev/null; echo $?)
check "BFS algorithm=bfs" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if d.get('algorithm')=='bfs' else 1)" 2>/dev/null; echo $?)
check "BFS path non-empty" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if len(d.get('path',[]))>0 else 1)" 2>/dev/null; echo $?)
check "BFS trace non-empty" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if len(d.get('trace',[]))>0 else 1)" 2>/dev/null; echo $?)

# --- DFS solve ---
echo ""
echo "--- Solve: DFS ---"
DFS_BODY='{"algorithm":"dfs","rows":5,"cols":5,"grid":[[0,0,0,0,0],[0,1,1,1,0],[0,0,0,1,0],[0,1,0,1,0],[0,1,0,0,0]],"start":{"row":0,"col":0},"end":{"row":4,"col":4}}'
CODE=$(api_post /api/solve "$DFS_BODY")
check "POST /api/solve DFS returns 200" $([ "$CODE" = "200" ]; echo $?)
check "DFS found=true" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if d.get('found') else 1)" 2>/dev/null; echo $?)

# --- Unreachable ---
echo ""
echo "--- Solve: unreachable ---"
UNREACH='{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,1,1],[1,1,1],[1,1,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
CODE=$(api_post /api/solve "$UNREACH")
check "unreachable returns 200" $([ "$CODE" = "200" ]; echo $?)
check "unreachable found=false" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if not d.get('found') else 1)" 2>/dev/null; echo $?)

# --- Generation ---
echo ""
echo "--- Maze generation ---"
GEN='{"rows":15,"cols":21}'
CODE=$(api_post /api/maze/generate "$GEN")
check "generate returns 200" $([ "$CODE" = "200" ]; echo $?)
check "generate returns grid" $(python3 -c "import json; d=json.load(open('/tmp/ml_body.json')); exit(0 if 'grid' in d else 1)" 2>/dev/null; echo $?)

# --- Error cases ---
echo ""
echo "--- Error cases ---"
CODE=$(api_post /api/solve '{bad json}')
check "malformed JSON returns 4xx" $([ "$CODE" -ge 400 ] 2>/dev/null; echo $?)

BAD_ALGO='{"algorithm":"xyz","rows":3,"cols":3,"grid":[[0,0,0],[0,0,0],[0,0,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
CODE=$(api_post /api/solve "$BAD_ALGO")
check "bad algorithm returns 4xx" $([ "$CODE" -ge 400 ] 2>/dev/null; echo $?)

CODE=$(api_get /api/unknown)
check "unknown route returns 404" $([ "$CODE" = "404" ]; echo $?)

# --- Summary ---
echo ""
echo -e "${CYAN}==========================================${NC}"
if [ "$FAIL" = "0" ]; then
    echo -e "${CYAN}  Results: ${GREEN}$PASS passed, $FAIL failed${NC}"
else
    echo -e "${CYAN}  Results: $PASS passed, ${RED}$FAIL failed${NC}"
fi
echo -e "${CYAN}==========================================${NC}"
echo ""
exit $([ "$FAIL" = "0" ]; echo $?)
