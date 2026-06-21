$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)
$plugin_path = "$dist\plugins\native\"
$pathdll = "$plugin_path\jsdec_cutter.dll"

if (-not (Test-Path -Path 'jsdec' -PathType Container)) {
    git clone https://github.com/rizinorg/jsdec.git --depth 1 --branch "dev"
}
cd jsdec
$jsdecdir = (Get-Item .).FullName

& meson.exe setup --buildtype=release -Dbuild_type=cutter "$jsdecdir\build_lib"
ninja -C "$jsdecdir\build_lib"

# cmake is silly and expects .lib but meson generates the static lib as .a
Copy-Item "$jsdecdir\build_lib\libjsdec.a" -Destination "$jsdecdir\build_lib\jsdec.lib"

mkdir build_plugin
cd build_plugin
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DJSDEC_BUILD_DIR="$jsdecdir\build_lib" -DCMAKE_INSTALL_PREFIX="$dist" -DCUTTER_INSTALL_PLUGDIR="plugins\native" $cmake_opts "$jsdecdir\cutter-plugin"
ninja install

$ErrorActionPreference = 'Stop'

if(![System.IO.File]::Exists($pathdll)) {
    echo "files: $plugin_path"
    ls "$plugin_path"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
