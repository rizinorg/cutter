@ECHO ON
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget https://storage.googleapis.com/chrome-infra/depot_tools.zip -OutFile depot_tools.zip "
7z -bd x %CD%\depot_tools.zip -odepot_tools
ECHO kek
depot_tools\update_depot_tools
ECHO kek
SET "BUFF_PATH=%PATH%"
ECHO kek
SET "DEPOT_TOOLS=%CD%\depot_tools"
ECHO kek
set "PATH=%DEPOT_TOOLS%%BUFF_PATH%"
ECHO kek
mkdir %CD%\breakpad
ECHO kek
SET "DAT_DIR=%CD%"
ECHO kek
CD %CD%\breakpad
ECHO kek
fetch breakpad
ECHO kek
gclient sync
ECHO kek
CD %DAT_DIR%
ECHO kek
DEL %DAT_DIR%\breakpad\src\src\client\windows\breakpad_client.gyp
ECHO kek
DEL %DAT_DIR%\breakpad\src\src\client\windows\breakpad_client.sln
ECHO kek
DEL %DAT_DIR%\breakpad\src\src\client\windows\common.vcxproj
ECHO kek
DEL %DAT_DIR%\breakpad\src\src\client\windows\common.vcxproj.filters
ECHO kek
DEL %DAT_DIR%\breakpad\src\src\client\windows\build_all.vcxproj
ECHO kek
COPY %CD%\scripts\breakpad_client.gyp %CD%\breakpad\src\src\client\windows
ECHO kek
CD %CD%\breakpad\src\src
ECHO kek
tools\gyp\gyp.bat --no-circular-check client\windows\breakpad_client.gyp -Dwin_release_RuntimeLibrary=2 -Dwin_debug_RuntimeLibrary=2  -Dplatform=%ARCH% -Dconfiguration=release
ECHO kek
set "PATH=%BUFF_PATH%"
ECHO kek
msbuild /m %CD%\client\windows\breakpad_client.sln /p:Configuration=release /p:Platform=%ARCH%
ECHO kek
CD %DAT_DIR%
ECHO kek
