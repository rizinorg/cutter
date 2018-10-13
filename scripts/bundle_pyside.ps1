$siteDir = $args[0]
$wheel = "PySide2-5.9.0a1.dev1528453054-5.9.6-cp35.cp36-none-win_amd64.whl"
$url = "https://download.qt.io/snapshots/ci/pyside/5.9/latest/pyside2/" + $wheel
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget $url -OutFile $wheel
python -m pip install -I --no-compile -t $siteDir $wheel

$fileList = @(
	"__init__.py",
	"_config.py",
	"_git_pyside_version.py",
	"pyside2.dll",
	"shiboken2.dll",
	"shiboken2.pyd",
	"QtCore.pyd",
	"QtCore.pyd",
	"QtGui.pyd",
	"QtWidgets.pyd",
	"QtNetwork.pyd",
	"Qt5Qml.dll",
	"QtQml.pyd"
)

$path = Join-Path -Path $siteDir -ChildPath "PySide2"
Get-ChildItem -Path $path -Exclude $fileList | Remove-Item -Force -Recurse
Remove-Item -Force $wheel