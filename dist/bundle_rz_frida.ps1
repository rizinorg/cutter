$dist = $args[0]
$cmake_opts = $args[1]

# keep in sync with rz-frida release
$FRIDA_VERSION = "17.17.0"

# url to be updated after rz-frida gets transferred under rizinorg
if (-not (Test-Path -Path 'rz-frida' -PathType Container)) {
    git clone https://github.com/IndAlok/rz-frida.git --depth 1 rz-frida
}
cd rz-frida
if (-not (Test-Path -Path 'frida-core-devkit' -PathType Container)) {
    Invoke-WebRequest -Uri "https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-core-devkit-$FRIDA_VERSION-windows-x86_64.tar.xz" -OutFile frida-core-devkit.tar.xz
    mkdir frida-core-devkit
    tar xf frida-core-devkit.tar.xz -C frida-core-devkit
}
& meson.exe --buildtype=release --prefix=$dist `
    -Dfrida_core=enabled `
    -Dfrida_include_dir="$PWD\frida-core-devkit" `
    -Dfrida_library="$PWD\frida-core-devkit\frida-core.lib" build
ninja -C build install
$pathdll = "$dist\lib\rizin\plugins\rz_frida.dll"
if(![System.IO.File]::Exists($pathdll)) {
    type build/meson-logs/meson-log.txt
    ls "$dist\lib\rizin\plugins\"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
Remove-Item -Recurse -Force "$dist\lib\rizin\plugins\rz_frida.lib"

cd plugin/cutter
mkdir build
cd build
$env:PKG_CONFIG_PATH = "$dist\lib\pkgconfig"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$dist" `
    -DCMAKE_PREFIX_PATH="$dist;$env:CUTTER_DEPS\qt" $cmake_opts ..
ninja
ninja install

$ErrorActionPreference = 'Stop'

$plugin_path = "$dist\plugins\native\"
$pathdll = "$plugin_path\cutter_frida_plugin.dll"

if(![System.IO.File]::Exists($pathdll)) {
    echo "files: $plugin_path"
    ls "$plugin_path"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
