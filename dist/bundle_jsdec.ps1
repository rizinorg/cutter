$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

if (-not (Test-Path -Path 'jsdec' -PathType Container)) {
    git clone https://github.com/rizinorg/jsdec.git --branch master --depth 1
}
cd jsdec
& meson.exe --buildtype=release -Dc_args=-DDUK_USE_DATE_NOW_WINDOWS --prefix=$dist --libdir=lib\plugins --datadir=lib\plugins p build
ninja -C build install
Remove-Item -Recurse -Force $dist\lib\plugins\core_pdd.lib
