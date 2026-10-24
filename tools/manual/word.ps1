# word.ps1 DOCX [PDF]: Word updates the manual's table of contents and fields and saves it; with PDF, it also exports
# the PDF (docs/Manual_Plan.md, M6: the PDF is Word's own). Word must be installed; it runs hidden.
# Word works on a copy with a new name each time, copied back only once it has saved. A Word that dies leaves the
# manual as it was, and Word's "it caused a serious error. Do you still want to open it?", which it shows on the
# screen even when hidden and then waits on, is kept for that copy's name, never opened again (22 Oct 2026).
param([Parameter(Mandatory = $true)][string]$Docx, [string]$Pdf)
$Docx = (Resolve-Path $Docx).Path
if ($Pdf) { $Pdf = [System.IO.Path]::GetFullPath($Pdf) }
$work = Join-Path $env:TEMP ('cmm2manual_' + [guid]::NewGuid().ToString('N') + '.docx')
Copy-Item $Docx $work
$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
try {
    $doc = $word.Documents.Open($work, $false, $false)
    foreach ($toc in $doc.TablesOfContents) { $toc.Update() }
    $doc.Fields.Update() | Out-Null
    $doc.Save()
    "pages: " + $doc.ComputeStatistics(2)
    if ($Pdf) {
        $doc.ExportAsFixedFormat($Pdf, 17, $false, 0, 0, 0, 0, 0, $true, $true, 1)   # PDF, with bookmarks from headings
        "pdf: " + $Pdf
    }
    $doc.Close($false)
    Copy-Item $work $Docx -Force
    "saved: " + $Docx
} finally {
    $word.Quit()
    Remove-Item $work -ErrorAction SilentlyContinue
}
# the reference sections' entries as bookmarks under their Detailed Listing headings, which Word's export does not make
# (bookmarks.py, as the PicoMite's add_manual_bookmarks.py; 24 Oct 2026)
if ($Pdf) { & python -I (Join-Path $PSScriptRoot 'bookmarks.py') $Docx $Pdf }
