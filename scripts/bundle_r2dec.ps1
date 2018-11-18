$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

git clone https://github.com/wargio/r2dec-js.git
cd r2dec-js
& $python\Scripts\meson.exe --buildtype=release -Dc_args=-DDUK_USE_DATE_NOW_WINDOWS p build
ninja -C build
Copy-Item . -Recurse -Destination $dist\radare2\lib\plugins\r2dec-js
Copy-Item build\core_pdd.dll -Destination $dist\radare2\lib\plugins
Remove-Item -Recurse -Force $dist\radare2\lib\plugins\r2dec-js\p
Remove-Item -Recurse -Force $dist\radare2\lib\plugins\r2dec-js\build
Remove-Item -Recurse -Force $dist\radare2\lib\plugins\r2dec-js\.git
Remove-Item -Recurse -Force $dist\radare2\lib\plugins\r2dec-js\.github
