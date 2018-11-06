$siteDir = $args[0]
python -m pip install -I --no-compile -t $siteDir --index-url=https://download.qt.io/snapshots/ci/pyside/5.9/latest/ pyside2 --trusted-host download.qt.io

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
