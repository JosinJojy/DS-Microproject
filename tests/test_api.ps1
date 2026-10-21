# MazeLab API Integration Tests — PowerShell
# Uses curl.exe with file bodies to avoid PowerShell quoting issues.
# Usage: .\tests\test_api.ps1 [port]
# Requires: mazelab.exe running on the given port

param([int]$Port = 8080)

$BASE   = "http://127.0.0.1:$Port"
$TMPDIR = "$env:TEMP\mazelab_test"
$pass   = 0
$fail   = 0

New-Item -ItemType Directory -Force -Path $TMPDIR | Out-Null

function Test-Case {
    param([string]$name, [bool]$cond)
    if ($cond) {
        Write-Host "  PASS  $name" -ForegroundColor Green
        $script:pass++
    } else {
        Write-Host "  FAIL  $name" -ForegroundColor Red
        $script:fail++
    }
}

# Write a JSON string to a temp file (no BOM)
function Write-Body([string]$json) {
    $file = "$TMPDIR\body.json"
    [System.IO.File]::WriteAllText($file, $json)
    return $file
}

# GET request — returns @{Status; Body}
function Api-Get([string]$path) {
    $out  = "$TMPDIR\resp.json"
    $code = curl.exe -s -o $out -w "%{http_code}" "$BASE$path" 2>$null
    $raw  = if (Test-Path $out) { Get-Content $out -Raw } else { "" }
    $parsed = try { $raw | ConvertFrom-Json } catch { $null }
    return @{ Status = [int]$code; Body = $parsed; Raw = $raw }
}

# POST request with JSON body file — returns @{Status; Body}
function Api-Post([string]$path, [string]$json) {
    $file = Write-Body $json
    $out  = "$TMPDIR\resp.json"
    $code = curl.exe -s -o $out -w "%{http_code}" --data-binary "@$file" -H "Content-Type: application/json" "$BASE$path" 2>$null
    $raw  = if (Test-Path $out) { Get-Content $out -Raw } else { "" }
    $parsed = try { $raw | ConvertFrom-Json } catch { $null }
    return @{ Status = [int]$code; Body = $parsed; Raw = $raw }
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  MazeLab API Integration Tests" -ForegroundColor Cyan
Write-Host "  Target: $BASE" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# ── Health ─────────────────────────────────────────────────────
Write-Host "`n--- Health endpoint ---"
$h = Api-Get "/api/health"
Test-Case "GET /api/health returns 200"        ($h.Status -eq 200)
Test-Case "health status=ok"                   ($h.Body.status -eq 'ok')
Test-Case "health app=MazeLab"                 ($h.Body.app -eq 'MazeLab')
Test-Case "health version present"             ($null -ne $h.Body.version)
$hm = Api-Post "/api/health" "{}"
Test-Case "POST /api/health returns 405"       ($hm.Status -eq 405)

# ── BFS Solve ──────────────────────────────────────────────────
Write-Host "`n--- Solve: BFS ---"
$rb = Api-Post "/api/solve" '{"algorithm":"bfs","rows":5,"cols":5,"grid":[[0,0,0,0,0],[0,1,1,1,0],[0,0,0,1,0],[0,1,0,1,0],[0,1,0,0,0]],"start":{"row":0,"col":0},"end":{"row":4,"col":4}}'
Test-Case "BFS returns 200"                    ($rb.Status -eq 200)
Test-Case "BFS found=true"                     ($rb.Body.found -eq $true)
Test-Case "BFS algorithm=bfs"                  ($rb.Body.algorithm -eq 'bfs')
Test-Case "BFS trace is array"                 ($rb.Body.trace -is [System.Array])
Test-Case "BFS path is array"                  ($rb.Body.path -is [System.Array])
Test-Case "BFS trace non-empty"                ($rb.Body.trace.Count -gt 0)
Test-Case "BFS path non-empty"                 ($rb.Body.path.Count -gt 0)
Test-Case "BFS visitedCount > 0"               (([int]$rb.Body.visitedCount) -gt 0)
Test-Case "BFS pathLength = pathCellCount - 1" (([int]$rb.Body.pathLength) -eq ([int]$rb.Body.pathCellCount - 1))
Test-Case "BFS path starts at (0,0)"           ($rb.Body.path[0].row -eq 0 -and $rb.Body.path[0].col -eq 0)
Test-Case "BFS path ends at (4,4)"             ($rb.Body.path[-1].row -eq 4 -and $rb.Body.path[-1].col -eq 4)
Test-Case "BFS executionTimeUs present"        ($null -ne $rb.Body.executionTimeUs)

# Verify BFS shortest path on open 3x3: path must be 5 cells (4 moves)
$rb2 = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,0,0],[0,0,0],[0,0,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
Test-Case "BFS open 3x3: pathCellCount=5"      (([int]$rb2.Body.pathCellCount) -eq 5)
Test-Case "BFS open 3x3: pathLength=4"         (([int]$rb2.Body.pathLength) -eq 4)

# ── DFS Solve ──────────────────────────────────────────────────
Write-Host "`n--- Solve: DFS ---"
$rd = Api-Post "/api/solve" '{"algorithm":"dfs","rows":5,"cols":5,"grid":[[0,0,0,0,0],[0,1,1,1,0],[0,0,0,1,0],[0,1,0,1,0],[0,1,0,0,0]],"start":{"row":0,"col":0},"end":{"row":4,"col":4}}'
Test-Case "DFS returns 200"                    ($rd.Status -eq 200)
Test-Case "DFS found=true"                     ($rd.Body.found -eq $true)
Test-Case "DFS algorithm=dfs"                  ($rd.Body.algorithm -eq 'dfs')
Test-Case "DFS trace non-empty"                ($rd.Body.trace.Count -gt 0)
Test-Case "DFS path non-empty"                 ($rd.Body.path.Count -gt 0)
Test-Case "DFS pathLength = pathCellCount - 1" (([int]$rd.Body.pathLength) -eq ([int]$rd.Body.pathCellCount - 1))
Test-Case "DFS path starts at (0,0)"           ($rd.Body.path[0].row -eq 0 -and $rd.Body.path[0].col -eq 0)
Test-Case "DFS path ends at (4,4)"             ($rd.Body.path[-1].row -eq 4 -and $rd.Body.path[-1].col -eq 4)

# ── Unreachable ────────────────────────────────────────────────
Write-Host "`n--- Solve: unreachable destination ---"
$ru = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,1,1],[1,1,1],[1,1,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
Test-Case "unreachable returns 200"            ($ru.Status -eq 200)
Test-Case "unreachable found=false"            ($ru.Body.found -eq $false)
Test-Case "unreachable path is empty"          ($ru.Body.path.Count -eq 0)
Test-Case "unreachable pathLength=0"           (([int]$ru.Body.pathLength) -eq 0)

# ── Start == End ───────────────────────────────────────────────
Write-Host "`n--- Solve: start equals end ---"
$rse = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,0,0],[0,0,0],[0,0,0]],"start":{"row":1,"col":1},"end":{"row":1,"col":1}}'
Test-Case "start==end returns 200"             ($rse.Status -eq 200)
Test-Case "start==end found=true"              ($rse.Body.found -eq $true)
Test-Case "start==end pathCellCount=1"         (([int]$rse.Body.pathCellCount) -eq 1)
Test-Case "start==end pathLength=0"            (([int]$rse.Body.pathLength) -eq 0)

# ── Maze generation ────────────────────────────────────────────
Write-Host "`n--- Maze generation ---"
$rg = Api-Post "/api/maze/generate" '{"rows":15,"cols":21}'
Test-Case "generate returns 200"               ($rg.Status -eq 200)
Test-Case "generate has rows"                  ($null -ne $rg.Body.rows)
Test-Case "generate has cols"                  ($null -ne $rg.Body.cols)
Test-Case "generate has grid array"            ($rg.Body.grid -is [System.Array])
Test-Case "generate grid row count matches"    ($rg.Body.grid.Count -eq [int]$rg.Body.rows)
Test-Case "generate has start"                 ($null -ne $rg.Body.start)
Test-Case "generate has end"                   ($null -ne $rg.Body.end)

if ($rg.Body -and $rg.Body.grid -and $rg.Body.start) {
    $sr = [int]$rg.Body.start.row; $sc = [int]$rg.Body.start.col
    $er = [int]$rg.Body.end.row;   $ec = [int]$rg.Body.end.col
    Test-Case "generate start cell open"       ($rg.Body.grid[$sr][$sc] -eq 0)
    Test-Case "generate end cell open"         ($rg.Body.grid[$er][$ec] -eq 0)

    # Verify generated maze is solvable
    $solveBody = @{
        algorithm = "bfs"
        rows      = [int]$rg.Body.rows
        cols      = [int]$rg.Body.cols
        grid      = $rg.Body.grid
        start     = $rg.Body.start
        end       = $rg.Body.end
    }
    $solveJson = $solveBody | ConvertTo-Json -Depth 10 -Compress
    $rgs = Api-Post "/api/solve" $solveJson
    Test-Case "generated maze BFS solvable"    ($rgs.Body.found -eq $true)
}

# ── Error cases ────────────────────────────────────────────────
Write-Host "`n--- Error cases ---"

$e1 = Api-Post "/api/solve" '{bad json}'
Test-Case "malformed JSON -> 400"              ($e1.Status -eq 400)

$e2 = Api-Post "/api/solve" '{"algorithm":"dijkstra","rows":3,"cols":3,"grid":[[0,0,0],[0,0,0],[0,0,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
Test-Case "bad algorithm -> 400"               ($e2.Status -eq 400)

$e3 = Api-Post "/api/solve" '{"algorithm":"bfs","rows":1,"cols":1,"grid":[[0]],"start":{"row":0,"col":0},"end":{"row":0,"col":0}}'
Test-Case "invalid dimensions -> 400"          ($e3.Status -eq 400)

$e4 = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[1,0,0],[0,0,0],[0,0,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
Test-Case "start on wall -> 400"               ($e4.Status -eq 400)

$e5 = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,0,0],[0,0,0],[0,0,1]],"start":{"row":0,"col":0},"end":{"row":2,"col":2}}'
Test-Case "end on wall -> 400"                 ($e5.Status -eq 400)

$e6 = Api-Post "/api/solve" '{"algorithm":"bfs"}'
Test-Case "missing fields -> 400"              ($e6.Status -eq 400)

$e7 = Api-Post "/api/solve" '{"algorithm":"bfs","rows":3,"cols":3,"grid":[[0,0],[0,0],[0,0]],"start":{"row":0,"col":0},"end":{"row":2,"col":1}}'
Test-Case "grid col mismatch -> 400"           ($e7.Status -eq 400)

$e8 = Api-Get "/api/unknown_route_xyz"
Test-Case "unknown route -> 404"               ($e8.Status -eq 404)

$e9 = Api-Get "/api/solve"
Test-Case "GET /api/solve -> 405"              ($e9.Status -eq 405)

$e10 = Api-Get "/api/maze/generate"
Test-Case "GET /api/maze/generate -> 405"      ($e10.Status -eq 405)

# ── Summary ────────────────────────────────────────────────────
Remove-Item -Path $TMPDIR -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
$color = if ($fail -eq 0) { 'Green' } else { 'Red' }
Write-Host "  Results: $pass passed, $fail failed" -ForegroundColor $color
Write-Host "==========================================" -ForegroundColor Cyan

exit $(if ($fail -eq 0) { 0 } else { 1 })
