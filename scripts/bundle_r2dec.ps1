$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

git clone https://github.com/radareorg/r2dec-js.git
cd r2dec-js
git checkout b5a0d15c7bcc488f268ffb0931b7ced2919f6c9d
& meson.exe --buildtype=release -Dc_args=-DDUK_USE_DATE_NOW_WINDOWS --prefix=$dist --libdir=lib\plugins --datadir=lib\plugins p build
ninja -C build install
Remove-Item -Recurse -Force $dist\lib\plugins\core_pdd.lib
