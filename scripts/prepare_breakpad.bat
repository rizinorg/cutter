powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget https://storage.googleapis.com/chrome-infra/depot_tools.zip -OutFile depot_tools.zip "
7z -bd x %CD%\depot_tools.zip -odepot_tools
depot_tools\update_depot_tools
SET "BUFF_PATH=%PATH%"
SET "DEPOT_TOOLS=%CD%\depot_tools"
setx PATH %DEPOT_TOOLS%%BUFF_PATH% \m
mkdir %CD%\breakpad
SET "DAT_DIR=%CD%"
CD %CD%\breakpad
fetch breakpad
gclient sync
CD %DAT_DIR%
DEL %DAT_DIR%\breakpad\src\src\client\windows\breakpad_client.gyp
DEL %DAT_DIR%\breakpad\src\src\client\windows\breakpad_client.sln
DEL %DAT_DIR%\breakpad\src\src\client\windows\common.vcxproj
DEL %DAT_DIR%\breakpad\src\src\client\windows\common.vcxproj.filters
DEL %DAT_DIR%\breakpad\src\src\client\windows\build_all.vcxproj
COPY %CD%\scripts\breakpad_client.gyp %CD%\breakpad\src\src\client\windows
CD %CD%\breakpad\src\src
tools\gyp\gyp.bat --no-circular-check client\windows\breakpad_client.gyp -Dwin_release_RuntimeLibrary=2 -Dwin_debug_RuntimeLibrary=2  -Dplatform=%ARCH% -Dconfiguration=release
setx PATH %BUFF_PATH% \m
msbuild /m %CD%\client\windows\breakpad_client.sln /p:Configuration=release /p:Platform=%ARCH%
CD %DAT_DIR%
