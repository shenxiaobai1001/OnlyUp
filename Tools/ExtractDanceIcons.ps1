Add-Type -AssemblyName System.Drawing

$srcPath = "C:\Users\Administrator\.cursor\projects\e-GitRepositories-OnlyUp\assets\c__Users_Administrator_AppData_Roaming_Cursor_User_workspaceStorage_a3ded9ace7cefb0079720a88f5f19db4_images_composer-annotation-e178c0a4-484d-4d14-b2c2-2812e38f4c27.png"
$outDir = "E:\GitRepositories\OnlyUp\Content\UI\DanceWheelUI\Icons"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$bmp = [System.Drawing.Bitmap]::FromFile($srcPath)
$w = $bmp.Width
$h = $bmp.Height

function Is-Red($c) {
    return ($c.R -gt 170 -and $c.G -lt 90 -and $c.B -lt 90 -and ($c.R - $c.G) -gt 70)
}
function Is-Yellow($c) {
    return ($c.R -gt 170 -and $c.G -gt 130 -and $c.B -lt 120 -and ($c.G - $c.B) -gt 40 -and $c.R -ge ($c.G - 30))
}
function Is-Light($c) {
    return ($c.R -gt 200 -and $c.G -gt 200 -and $c.B -gt 180)
}
function Is-Dark($c) {
    return ($c.R -lt 70 -and $c.G -lt 70 -and $c.B -lt 70)
}

$red = New-Object 'System.Collections.Generic.List[System.Drawing.Point]'
for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) {
        $c = $bmp.GetPixel($x, $y)
        if (Is-Red $c) { $red.Add([System.Drawing.Point]::new($x, $y)) }
    }
}
Write-Output "red pixels: $($red.Count)"

# Cluster red pixels by proximity into two circles.
$visited = @{}
$clusters = @()
foreach ($p in $red) {
    $key = "$($p.X),$($p.Y)"
    if ($visited.ContainsKey($key)) { continue }
    $queue = New-Object System.Collections.Generic.Queue[System.Drawing.Point]
    $queue.Enqueue($p)
    $visited[$key] = $true
    $pts = New-Object System.Collections.Generic.List[System.Drawing.Point]
    while ($queue.Count -gt 0) {
        $cur = $queue.Dequeue()
        $pts.Add($cur)
        for ($dy = -3; $dy -le 3; $dy++) {
            for ($dx = -3; $dx -le 3; $dx++) {
                if ($dx -eq 0 -and $dy -eq 0) { continue }
                $nx = $cur.X + $dx
                $ny = $cur.Y + $dy
                if ($nx -lt 0 -or $ny -lt 0 -or $nx -ge $w -or $ny -ge $h) { continue }
                $nk = "$nx,$ny"
                if ($visited.ContainsKey($nk)) { continue }
                if (-not (Is-Red ($bmp.GetPixel($nx, $ny)))) { continue }
                $visited[$nk] = $true
                $queue.Enqueue([System.Drawing.Point]::new($nx, $ny))
            }
        }
    }
    if ($pts.Count -gt 40) { $clusters += ,$pts }
}
Write-Output "red clusters: $($clusters.Count)"

function Export-Icon($pts, $name) {
    $minX = ($pts | Measure-Object X -Minimum).Minimum
    $maxX = ($pts | Measure-Object X -Maximum).Maximum
    $minY = ($pts | Measure-Object Y -Minimum).Minimum
    $maxY = ($pts | Measure-Object Y -Maximum).Maximum
    $cx = [int](($minX + $maxX) / 2)
    $cy = [int](($minY + $maxY) / 2)
    Write-Output "$name circle bbox $minX,$minY-$maxX,$maxY center $cx,$cy"

    # Yellow pixels inside the circle's bounding box, expanded a bit.
    $pad = 8
    $x0 = [Math]::Max(0, $minX - $pad)
    $y0 = [Math]::Max(0, $minY - $pad)
    $x1 = [Math]::Min($w - 1, $maxX + $pad)
    $y1 = [Math]::Min($h - 1, $maxY + $pad)

    $ys = @()
    for ($y = $y0; $y -le $y1; $y++) {
        for ($x = $x0; $x -le $x1; $x++) {
            if (Is-Yellow ($bmp.GetPixel($x, $y))) { $ys += [System.Drawing.Point]::new($x, $y) }
        }
    }
    if ($ys.Count -eq 0) { Write-Output "no yellow"; return }

    # Keep the yellow component closest to the circle center.
    $seen = @{}
    $best = $null
    $bestDist = [int]::MaxValue
    foreach ($seed in $ys) {
        $sk = "$($seed.X),$($seed.Y)"
        if ($seen.ContainsKey($sk)) { continue }
        $q = New-Object System.Collections.Generic.Queue[System.Drawing.Point]
        $q.Enqueue($seed)
        $seen[$sk] = $true
        $comp = New-Object System.Collections.Generic.List[System.Drawing.Point]
        $sx = 0; $sy = 0
        while ($q.Count -gt 0) {
            $cur = $q.Dequeue()
            $comp.Add($cur)
            $sx += $cur.X; $sy += $cur.Y
            foreach ($d in @(@(-1,0),@(1,0),@(0,-1),@(0,1),@(-1,-1),@(1,-1),@(-1,1),@(1,1))) {
                $nx = $cur.X + $d[0]; $ny = $cur.Y + $d[1]
                if ($nx -lt $x0 -or $ny -lt $y0 -or $nx -gt $x1 -or $ny -gt $y1) { continue }
                $nk = "$nx,$ny"
                if ($seen.ContainsKey($nk)) { continue }
                if (-not (Is-Yellow ($bmp.GetPixel($nx, $ny)))) { continue }
                $seen[$nk] = $true
                $q.Enqueue([System.Drawing.Point]::new($nx, $ny))
            }
        }
        if ($comp.Count -lt 80) { continue }
        $ccx = [int]($sx / $comp.Count)
        $ccy = [int]($sy / $comp.Count)
        $dist = ($ccx - $cx) * ($ccx - $cx) + ($ccy - $cy) * ($ccy - $cy)
        if ($dist -lt $bestDist) { $bestDist = $dist; $best = $comp }
    }
    if ($null -eq $best) { Write-Output "no component"; return }

    $bx0 = ($best | Measure-Object X -Minimum).Minimum
    $bx1 = ($best | Measure-Object X -Maximum).Maximum
    $by0 = ($best | Measure-Object Y -Minimum).Minimum
    $by1 = ($best | Measure-Object Y -Maximum).Maximum
    $m = 3
    $bx0 = [Math]::Max(0, $bx0 - $m)
    $by0 = [Math]::Max(0, $by0 - $m)
    $bx1 = [Math]::Min($w - 1, $bx1 + $m)
    $by1 = [Math]::Min($h - 1, $by1 + $m)
    $cw = $bx1 - $bx0 + 1
    $ch = $by1 - $by0 + 1
    Write-Output "$name icon $cw x $ch at $bx0,$by0"

    $out = New-Object System.Drawing.Bitmap $cw, $ch, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $yellowSet = @{}
    foreach ($pt in $best) { $yellowSet["$($pt.X - $bx0),$($pt.Y - $by0)"] = $true }

    for ($y = 0; $y -lt $ch; $y++) {
        for ($x = 0; $x -lt $cw; $x++) {
            $c = $bmp.GetPixel($x + $bx0, $y + $by0)
            $keep = $false
            if (Is-Yellow $c) { $keep = $true }
            elseif ((Is-Dark $c) -or (Is-Light $c)) {
                $near = $false
                for ($dy = -2; $dy -le 2 -and -not $near; $dy++) {
                    for ($dx = -2; $dx -le 2 -and -not $near; $dx++) {
                        if ($yellowSet.ContainsKey("$($x+$dx),$($y+$dy)")) { $near = $true }
                    }
                }
                if ($near) { $keep = $true }
            }
            if (-not $keep) {
                $out.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(0, 0, 0, 0))
            } else {
                $out.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $c.R, $c.G, $c.B))
            }
        }
    }

    # Fill light pixels (the digit) with neighboring yellow, a few passes.
    for ($pass = 0; $pass -lt 12; $pass++) {
        $pending = @()
        for ($y = 1; $y -lt $ch - 1; $y++) {
            for ($x = 1; $x -lt $cw - 1; $x++) {
                $c = $out.GetPixel($x, $y)
                if ($c.A -eq 0) { continue }
                if (-not (Is-Light $c)) { continue }
                $sr = 0; $sg = 0; $sb = 0; $n = 0
                foreach ($d in @(@(-1,0),@(1,0),@(0,-1),@(0,1),@(-1,-1),@(1,-1),@(-1,1),@(1,1))) {
                    $nc = $out.GetPixel($x + $d[0], $y + $d[1])
                    if ($nc.A -gt 0 -and (Is-Yellow $nc)) {
                        $sr += $nc.R; $sg += $nc.G; $sb += $nc.B; $n++
                    }
                }
                if ($n -gt 0) {
                    $pending += ,@($x, $y, [int]($sr / $n), [int]($sg / $n), [int]($sb / $n))
                }
            }
        }
        foreach ($item in $pending) {
            $out.SetPixel($item[0], $item[1], [System.Drawing.Color]::FromArgb(255, $item[2], $item[3], $item[4]))
        }
        Write-Output "$name pass $pass filled $($pending.Count)"
        if ($pending.Count -eq 0) { break }
    }

    $dest = Join-Path $outDir $name
    $out.Save($dest, [System.Drawing.Imaging.ImageFormat]::Png)
    $out.Dispose()
    Write-Output "saved $dest"
}

$i = 0
foreach ($cluster in ($clusters | Sort-Object { $_.Count } -Descending | Select-Object -First 2)) {
    $i++
    $cy = ($cluster | Measure-Object Y -Average).Average
    $label = if ($cy -lt ($h / 2)) { "Icon_ArmsUp.png" } else { "Icon_HandsOnHips.png" }
    Export-Icon $cluster $label
}

$bmp.Dispose()
