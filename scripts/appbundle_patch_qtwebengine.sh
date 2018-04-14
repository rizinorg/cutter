#!/bin/bash

if ! [[ $# -eq 1 ]]; then
    echo "Usage: $0 [AppBundle.app]"
    exit 1
fi

appbundle=$1
qtwebegineprocess="$1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess"

echo "Patching $appbundle to fix QtWebEngine"

if ! [[ -f "$qtwebegineprocess" ]]; then
	echo "$qtwebegineprocess does not exist. Did you forget to run macdeployqt?"
	exit 1
fi

install_name_tool `otool -L "$qtwebegineprocess" | sed -n "s/^[[:blank:]]*\(\/usr\/local\/Cellar[^[:blank:]]*\(Qt[A-Za-z]*\.framework[^[:blank:]]*\)\) (.*$/-change \1 @executable_path\/..\/..\/..\/..\/..\/..\/..\/\2/p"` \
    $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/MacOS/QtWebEngineProcess \
    || exit 1

mkdir -p $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks
ln -s ../../../../../../../QtCore.framework        $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtQuick.framework       $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtGui.framework         $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtQml.framework         $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtNetwork.framework     $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtWebChannel.framework  $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/
ln -s ../../../../../../../QtPositioning.framework $1/Contents/Frameworks/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app/Contents/Frameworks/ 
      
      
      
      
