
import sys
import os
import requests
import re
import subprocess

fetch_deps_path = os.path.join(os.path.dirname(sys.argv[0]), "fetch_deps.sh")

print("Fetching latest release")
json = requests.get("https://api.github.com/repos/radareorg/cutter-deps/releases/latest").json()

release_url = json["assets"][0]["browser_download_url"]

print(f"Getting MD5 for {release_url}")

curl = subprocess.Popen(["curl", "-L", release_url], stdout=subprocess.PIPE)
md5sum = subprocess.run(["md5sum"], stdin=curl.stdout, capture_output=True, encoding="utf-8").stdout
curl.wait()

md5sum = re.match("([a-zA-Z0-9]+) ", md5sum).group(1)

print(f"MD5: {md5sum}")

with open(fetch_deps_path) as f:
	fetch_deps = f.read()

fetch_deps = re.sub("^URL=.*$", f"URL={release_url}".replace("\\", r"\\"), fetch_deps, flags=re.MULTILINE)
fetch_deps = re.sub("^MD5=.*$", f"MD5={md5sum}".replace("\\", r"\\"), fetch_deps, flags=re.MULTILINE)

with open(fetch_deps_path, "w") as f:
	f.write(fetch_deps)
