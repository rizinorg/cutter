$dist = $args[0]
$cmake_opts = $args[1]
$python = Split-Path((Get-Command python.exe).Path)

if (-not (Test-Path -Path 'rz-silhouette' -PathType Container)) {
    git clone https://github.com/rizinorg/rz-silhouette.git --depth 1 rz-silhouette
}
cd rz-silhouette
& meson.exe --buildtype=release --prefix=$dist build
ninja -C build install
$pathdll = "$dist\lib\rizin\plugins\rz_silhouette.dll"
if(![System.IO.File]::Exists($pathdll)) {
    type build/meson-logs/meson-log.txt
    ls "$dist\lib\rizin\plugins\"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
Remove-Item -Recurse -Force "$dist\lib\rizin\plugins\rz_silhouette.lib"
