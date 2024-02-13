$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

if (-not (Test-Path -Path 'jsdec' -PathType Container)) {
    git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch "v0.7.0"
}
cd jsdec
& meson.exe --buildtype=release --prefix="$dist" build
ninja -C build install
$ErrorActionPreference = 'Stop'
$pathdll = "$dist\lib\rizin\plugins\core_pdd.dll"
if(![System.IO.File]::Exists($pathdll)) {
    type build\meson-logs\meson-log.txt
    ls "$dist\lib\rizin\plugins\"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
Remove-Item -Recurse -Force "$dist\lib\rizin\plugins\core_pdd.lib"
