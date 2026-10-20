# word.ps1 DOCX [PDF]: Word updates the manual's table of contents and fields and saves it; with PDF, it also exports
# the PDF (docs/Manual_Plan.md, M6: the PDF is Word's own). Word must be installed; it runs hidden.
param([Parameter(Mandatory = $true)][string]$Docx, [string]$Pdf)
$Docx = (Resolve-Path $Docx).Path
$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
try {
    $doc = $word.Documents.Open($Docx, $false, $false)
    foreach ($toc in $doc.TablesOfContents) { $toc.Update() }
    $doc.Fields.Update() | Out-Null
    $doc.Save()
    "pages: " + $doc.ComputeStatistics(2)
    if ($Pdf) {
        $doc.ExportAsFixedFormat($Pdf, 17, $false, 0, 0, 0, 0, 0, $true, $true, 1)   # PDF, with bookmarks from headings
        "pdf: " + $Pdf
    }
    $doc.Close($false)
} finally {
    $word.Quit()
}
