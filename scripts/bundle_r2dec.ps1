$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

git clone --depth 1 https://github.com/radareorg/r2dec-js.git --branch 4.5.0
cd r2dec-js
& meson.exe --buildtype=release -Dc_args=-DDUK_USE_DATE_NOW_WINDOWS --prefix=$dist --libdir=lib\plugins --datadir=lib\plugins p build
ninja -C build install
Remove-Item -Recurse -Force $dist\lib\plugins\core_pdd.lib
