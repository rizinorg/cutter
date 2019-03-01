powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget https://storage.googleapis.com/chrome-infra/depot_tools.zip -OutFile depot_tools.zip "
7z -bd x %CD%\depot_tools.zip -odepot_tools
%CD%\depot_tools\update_depot_tools
ECHO KEK
set "PATH=%CD%\depot_tools;%PATH%"
DIR
mkdir %CD%\breakpad
DIR
cd %CD%\breakpad
ECHO klsejgseklgjseklgjseklgjseklgjseklgjsel
DIR
fetch breakpad
gclient sync
ECHO klsejgseklgjseklgjseklgjseklgjseklgjsel 22
DIR
cd ..

msbuild /m %CD%\breakpad\src\src\client\windows\handler\exception_handler.vcxproj /p:Configuration=release
