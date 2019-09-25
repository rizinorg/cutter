$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

git clone https://github.com/wargio/r2dec-js.git
cd r2dec-js
& $python\Scripts\meson.exe --buildtype=release -Dc_args=-DDUK_USE_DATE_NOW_WINDOWS --prefix=$dist\radare2 --libdir=lib\plugins --datadir=lib\plugins p build
ninja -C build install
Remove-Item -Recurse -Force $dist\radare2\lib\plugins\core_pdd.lib
