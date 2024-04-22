$dist = $args[0]
$python = Split-Path((Get-Command python.exe).Path)

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
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DJSDEC_BUILD_DIR="$jsdecdir\build_lib" -DCMAKE_INSTALL_PREFIX="$dist" $cmake_opts "$jsdecdir\cutter-plugin"
ninja install

$ErrorActionPreference = 'Stop'
$pathdll = "$dist\share\rizin\cutter\plugins\native\jsdec_cutter.dll"
if(![System.IO.File]::Exists($pathdll)) {
    echo "files: $dist\share\rizin\cutter\plugins\native\"
    ls "$dist\share\rizin\cutter\plugins\native\"
    throw (New-Object System.IO.FileNotFoundException("File not found: $pathdll", $pathdll))
}
