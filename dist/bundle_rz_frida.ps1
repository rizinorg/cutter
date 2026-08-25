$ErrorActionPreference = 'Stop'
$env:GIT_TERMINAL_PROMPT = '0'

if ($args.Count -lt 1 -or $args.Count -gt 2) {
    throw "Usage: bundle_rz_frida.ps1 <install-prefix> [extra-cmake-opts]"
}
$dist = $args[0]
$cmake_opts = $args[1]
if (-not $dist) {
    throw "install prefix is empty"
}

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
    $ok = $false
    if (Get-Command 7z.exe -ErrorAction SilentlyContinue) {
        7z.exe x -y "-ofrida-core-devkit" frida-core-devkit.tar.xz | Out-Null
        $ok = Test-Path -Path 'frida-core-devkit\frida-core.h' -PathType Leaf
        if (-not $ok) {
            foreach ($innerTar in @('frida-core-devkit.tar', 'frida-core-devkit\frida-core-devkit.tar')) {
                if (Test-Path -Path $innerTar -PathType Leaf) {
                    7z.exe x -y "-ofrida-core-devkit" $innerTar | Out-Null
                    Remove-Item -Force $innerTar -ErrorAction SilentlyContinue
                    $ok = Test-Path -Path 'frida-core-devkit\frida-core.h' -PathType Leaf
                    if ($ok) { break }
                }
            }
        }
    }
    # sys32 tar hangs: (https://github.com/rizinorg/cutter/actions/runs/31564868702/job/94014574770)
    # so we skip it if that's on path.
    if (-not $ok) {
        $tar = Get-Command tar.exe -ErrorAction SilentlyContinue
        if ($tar -and $tar.Source -notmatch '(?i)[\\/]System32[\\/]tar\.exe$') {
            & $tar.Source xf frida-core-devkit.tar.xz -C frida-core-devkit
            $ok = Test-Path -Path 'frida-core-devkit\frida-core.h' -PathType Leaf
        }
    }
    Remove-Item -Force 'frida-core-devkit.tar', 'frida-core-devkit\frida-core-devkit.tar' -ErrorAction SilentlyContinue
    if (-not $ok) {
        Get-ChildItem -Recurse frida-core-devkit | Select-Object -First 20 FullName
        throw "frida-core devkit extraction failed: frida-core.h not found in frida-core-devkit"
    }
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
}

Remove-Item -Force "$dist\lib\rizin\plugins\rz_frida.lib" -ErrorAction SilentlyContinue
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
