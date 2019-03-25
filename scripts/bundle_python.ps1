$arch = $args[0]
$dist = $args[1]

$py_version = (python --version).Split()[1]
$py_base = "python" + $py_version[0] + $py_version[2]
$py_platform = If ($arch -eq "x64") {"amd64"} Else {"win32"}
$py_url = "https://www.python.org/ftp/python/${py_version}/python-${py_version}-embed-${py_platform}.zip"

[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; wget ${py_url} -OutFile python_embed.zip; Expand-Archive .\python_embed.zip -DestinationPath .\python_embed
New-Item -ItemType directory -Force -Path $dist\$py_base
Copy-Item .\python_embed\${py_base}.zip -Destination $dist\$py_base
Copy-Item .\python_embed\*.pyd -Destination $dist\$py_base
Copy-Item .\python_embed\sqlite3.dll -Destination $dist\$py_base
Copy-Item .\python_embed\python*.dll -Destination $dist
[System.IO.File]::WriteAllLines("${dist}\${py_base}._pth", "${py_base}`r`n${py_base}\${py_base}.zip`r`n${py_base}\site-packages")
