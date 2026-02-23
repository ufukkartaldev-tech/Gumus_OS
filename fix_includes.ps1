$files = Get-ChildItem -Path src/kernel -Recurse -Include *.c,*.h
foreach ($file in $files) {
    $content = Get-Content $file.FullName
    $newContent = @()
    foreach ($line in $content) {
        if ($line -match '#include\s*\"') {
            # Extract the filename only if there's a path
            $line = $line -replace '#include\s*\"([^"]*/)+([^/"]+\.h)\"', '#include "$2"'
        }
        $newContent += $line
    }
    $newContent | Set-Content $file.FullName -Encoding utf8
}
