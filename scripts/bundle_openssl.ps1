$arch = $args[0]
$dist = $args[1]

$libpath = If ($arch -eq "x64") {"C:\OpenSSL-Win64\bin"} Else {"C:\OpenSSL-Win32\bin"}
Copy-Item $libpath\libeay32.dll -Destination $dist
Copy-Item $libpath\ssleay32.dll -Destination $dist
