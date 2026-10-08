$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $project 'build\main.exe'
if (-not (Test-Path -LiteralPath $exe)) {
    throw 'Build first by running .\build.cmd from the project folder.'
}
$testDir = Join-Path $project 'build\test-inputs'
New-Item -ItemType Directory -Path $testDir -Force | Out-Null
$utf8 = New-Object System.Text.UTF8Encoding($false)
$cases = @(
    @{ Name = 'nested'; Xml = '<root><a><b>text</b></a></root>'; Code = 0 },
    @{ Name = 'same-name'; Xml = '<a><a></a></a>'; Code = 0 },
    @{ Name = 'empty-element'; Xml = '<root />'; Code = 0 },
    @{ Name = 'attributes'; Xml = '<root test="a > b" other=''value''><x/></root>'; Code = 0 },
    @{ Name = 'whitespace'; Xml = "`r`n<root>`r`n<x></x   >`r`n</root>`r`n"; Code = 0 },
    @{ Name = 'comment'; Xml = '<!-- <fake> --><root><!-- </root> --></root><!-- tail -->'; Code = 0 },
    @{ Name = 'cdata'; Xml = '<root><![CDATA[<fake></other>]]></root>'; Code = 0 },
    @{ Name = 'instructions'; Xml = '<?xml version="1.0"?><root><?work <ignored> ?></root>'; Code = 0 },
    @{ Name = 'prefix'; Xml = '<ns:root><ns:item/></ns:root>'; Code = 0 },
    @{ Name = 'long-file'; Xml = '<root>' + ('text' * 4000) + '</root>'; Code = 0 },
    @{ Name = 'utf8-bom'; Xml = [string][char]0xFEFF + '<root/>'; Code = 0 },
    @{ Name = 'maximum-depth'; Xml = ('<a>' * 1024) + ('</a>' * 1024); Code = 0 },
    @{ Name = 'maximum-name'; Xml = '<' + ('a' * 255) + '/>'; Code = 0 },
    @{ Name = 'missing'; Xml = "<root>`n<a></root>"; Code = 1; Detail = 'Expected </a>' },
    @{ Name = 'crossed'; Xml = '<root><a><b></a></b></root>'; Code = 1 },
    @{ Name = 'unclosed'; Xml = '<root><a>'; Code = 1; Detail = 'Missing </a>' },
    @{ Name = 'stray-close'; Xml = '</root>'; Code = 1 },
    @{ Name = 'case-sensitive'; Xml = '<Root></root>'; Code = 1 },
    @{ Name = 'two-roots'; Xml = '<a/><b/>'; Code = 1 },
    @{ Name = 'trailing-close'; Xml = '<root/></root>'; Code = 1 },
    @{ Name = 'outside-text'; Xml = 'hello<root/>'; Code = 1 },
    @{ Name = 'trailing-text'; Xml = '<root/>hello'; Code = 1 },
    @{ Name = 'empty-file'; Xml = ''; Code = 1 },
    @{ Name = 'comment-only'; Xml = '<!-- hello -->'; Code = 1 },
    @{ Name = 'bad-name'; Xml = '<1root/>'; Code = 1 },
    @{ Name = 'space-after-open'; Xml = '< root/>'; Code = 1 },
    @{ Name = 'truncated-tag'; Xml = '<root'; Code = 1 },
    @{ Name = 'bad-close'; Xml = '<root></root extra>'; Code = 1 },
    @{ Name = 'unquoted-attribute'; Xml = '<root x=word/>'; Code = 1 },
    @{ Name = 'unclosed-quote'; Xml = '<root x="word>'; Code = 1 },
    @{ Name = 'attribute-no-space'; Xml = '<root x="one"y="two"/>'; Code = 1 },
    @{ Name = 'attribute-less-than'; Xml = '<root x="<"/>'; Code = 1 },
    @{ Name = 'bad-empty-tag'; Xml = '<root / >'; Code = 1 },
    @{ Name = 'unclosed-comment'; Xml = '<root><!-- text</root>'; Code = 1 },
    @{ Name = 'comment-dashes'; Xml = '<root><!-- a--b --></root>'; Code = 1 },
    @{ Name = 'unclosed-cdata'; Xml = '<root><![CDATA[text</root>'; Code = 1 },
    @{ Name = 'outside-cdata'; Xml = '<![CDATA[text]]><root/>'; Code = 1 },
    @{ Name = 'unclosed-instruction'; Xml = '<?xml version="1.0"><root/>'; Code = 1 },
    @{ Name = 'doctype-unsupported'; Xml = '<!DOCTYPE root><root/>'; Code = 1; Detail = 'not supported' },
    @{ Name = 'cdata-end-in-text'; Xml = '<root>text]]></root>'; Code = 1 },
    @{ Name = 'nul-byte'; Xml = '<root>' + [char]0 + '</root>'; Code = 1 },
    @{ Name = 'excessive-depth'; Xml = ('<a>' * 1025) + ('</a>' * 1025); Code = 1 },
    @{ Name = 'excessive-name'; Xml = '<' + ('a' * 256) + '/>'; Code = 1 }
)
$passed = 0
foreach ($case in $cases) {
    $path = Join-Path $testDir ($case.Name + '.xml')
    [System.IO.File]::WriteAllText($path, $case.Xml, $utf8)
    $output = (& $exe $path | Out-String)
    $actualCode = $LASTEXITCODE
    $expectedText = if ($case.Code -eq 0) { 'XML is valid' } else { 'XML is invalid' }
    if ($actualCode -ne $case.Code -or -not $output.Contains($expectedText) -or
        ($case.Detail -and -not $output.Contains($case.Detail))) {
        throw "FAIL $($case.Name): expected exit $($case.Code), got $actualCode. $output"
    }
    $passed++
}
# File/usage errors go to stderr. Use Start-Process to capture them separately.
foreach ($arguments in @('build\test-inputs\does-not-exist.xml', 'one.xml two.xml')) {
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $project -WindowStyle Hidden -Wait -PassThru -RedirectStandardError (Join-Path $testDir 'stderr.txt') -RedirectStandardOutput (Join-Path $testDir 'stdout.txt')
    if ($process.ExitCode -ne 2) { throw 'Expected exit 2 for a file or usage error.' }
    $passed++
}
Push-Location $project
try {
    $output = (& $exe | Out-String)
    if ($LASTEXITCODE -ne 0 -or -not $output.Contains('XML is valid')) {
        throw 'Default note.xml did not pass.'
    }
    $passed++
} finally {
    Pop-Location
}
Write-Host "All $passed XML validator checks passed."
