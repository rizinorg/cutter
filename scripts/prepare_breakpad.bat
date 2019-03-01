powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget https://storage.googleapis.com/chrome-infra/depot_tools.zip -OutFile depot_tools.zip "
7z -bd x %CD%\depot_tools.zip -odepot_tools
depot_tools\update_depot_tools
set "PATH=%CD%\depot_tools;%PATH%"
  
mkdir %CD%\breakpad
cd %CD%\breakpad
fetch breakpad
gclient sync
cd ..

msbuild /m %CD%\breakpad\src\src\client\windows\handler\exception_handler.vcxproj /p:Configuration=release
