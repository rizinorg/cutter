@ECHO ON
SET ROOT_DIR=%CD%

powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget https://storage.googleapis.com/chrome-infra/depot_tools.zip -OutFile depot_tools.zip "
7z -bd x %ROOT_DIR%\depot_tools.zip -odepot_tools
powershell -Command "depot_tools\update_depot_tools"
SET BUFF_PATH=%PATH%
SET DEPOT_TOOLS=%ROOT_DIR%\depot_tools
set PATH=%DEPOT_TOOLS%;%BUFF_PATH%

mkdir %ROOT_DIR%\breakpad
CD %ROOT_DIR%\breakpad
powershell -Command "fetch breakpad"
powershell -Command "gclient sync"

CD %ROOT_DIR%\breakpad\src\src\client\windows
DEL %CD%\breakpad_client.gyp
DEL %CD%\breakpad_client.sln
DEL %CD%\common.vcxproj
DEL %CD%\common.vcxproj.filters
DEL %CD%\build_all.vcxproj

CD %ROOT_DIR%\breakpad\src\src\tools\windows\dump_syms
DEL %CD%\dump_syms.gyp
DEL %CD%\dump_syms.vcxproj
DEL %CD%\dump_syms.vcproj
DEL %CD%\dump_syms.sln

COPY %ROOT_DIR%\scripts\breakpad_client.gyp %ROOT_DIR%\breakpad\src\src\client\windows
COPY %ROOT_DIR%\scripts\dump_syms.gyp %ROOT_DIR%\breakpad\src\src\tools\windows\dump_syms
CD %ROOT_DIR%\breakpad\src\src
powershell -Command "tools\gyp\gyp.bat --no-circular-check client\windows\breakpad_client.gyp -Dwin_release_RuntimeLibrary=2 -Dwin_debug_RuntimeLibrary=2  -Dplatform=%ARCH% -Dconfiguration=release"
powershell -Command "tools\gyp\gyp.bat --no-circular-check tools\windows\dump_syms\dump_syms.gyp -Dwin_release_RuntimeLibrary=2 -Dwin_debug_RuntimeLibrary=2  -Dplatform=%ARCH% -Dconfiguration=release"

set PATH=%BUFF_PATH%
msbuild /m %CD%\client\windows\breakpad_client.sln /p:Configuration=release /p:Platform=%ARCH%
msbuild /m %CD%\tools\windows\dump_syms\dump_syms.sln /p:Configuration=release /p:Platform=%ARCH%
CD %ROOT_DIR%
