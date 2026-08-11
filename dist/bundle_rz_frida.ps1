$ErrorActionPreference = 'Stop'
$env:GIT_TERMINAL_PROMPT = '0'

$dist = $args[0]
$cmake_opts = $args[1]

# keep in sync with rz-frida release
$FRIDA_VERSION = "17.17.0"

if (-not (Test-Path -Path 'rz-frida\meson.build' -PathType Leaf)) {
    git clone https://github.com/rizinorg/rz-frida.git --depth 1 rz-frida
}
cd rz-frida
if (-not (Test-Path -Path 'frida-core-devkit\frida-core.h' -PathType Leaf)) {
    curl.exe -L --retry 3 --connect-timeout 30 --max-time 900 `
        -o frida-core-devkit.tar.xz `
        "https://github.com/frida/frida/releases/download/$FRIDA_VERSION/frida-core-devkit-$FRIDA_VERSION-windows-x86_64.tar.xz"
    New-Item -ItemType Directory -Force -Path frida-core-devkit | Out-Null
    tar xf frida-core-devkit.tar.xz -C frida-core-devkit
}

$env:PKG_CONFIG_PATH = "$PWD\..\lib\pkgconfig"
if (Test-Path -Path 'build' -PathType Container) {
    Remove-Item -Recurse -Force build
}
& meson.exe --buildtype=release --prefix=$dist `
    -Dfrida_core=enabled `
    -Dfrida_include_dir="$PWD\frida-core-devkit" `
    -Dfrida_library="$PWD\frida-core-devkit\frida-core.lib" build
ninja -C build install

New-Item -ItemType Directory -Force -Path "$dist\lib\rizin\plugins" | Out-Null
$rzFridaSrc = "$PWD\..\lib\rizin\plugins\rz_frida.dll"
if (Test-Path -Path $rzFridaSrc -PathType Leaf) {
    Copy-Item -Force $rzFridaSrc "$dist\lib\rizin\plugins\"
    Remove-Item -Recurse -Force "$PWD\..\lib\rizin\plugins\rz_frida.lib" -ErrorAction SilentlyContinue
}
$pathdll = "$dist\lib\rizin\plugins\rz_frida.dll"
if(![System.IO.File]::Exists($pathdll)) {
    type build/meson-logs/meson-log.txt
    ls "$PWD\..\lib\rizin\plugins\" -ErrorAction SilentlyContinue
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}

cd plugin/cutter
New-Item -ItemType Directory -Force -Path build | Out-Null
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$dist" `
    -DCMAKE_PREFIX_PATH="$dist;$env:CUTTER_DEPS\qt" $cmake_opts ..
ninja
ninja install

$plugin_path = "$dist\plugins\native\"
$pathdll = "$plugin_path\cutter_frida_plugin.dll"

if(![System.IO.File]::Exists($pathdll)) {
    echo "files: $plugin_path"
    ls "$plugin_path"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
