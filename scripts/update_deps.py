
import sys
import os
import requests
import re
import subprocess

platforms = ["linux", "macos", "win"]

if sys.platform == "darwin":
	md5sum_cmd = ["md5", "-r"]
else:
	md5sum_cmd = ["md5sum"]

fetch_deps_path = os.path.join(os.path.dirname(sys.argv[0]), "fetch_deps.sh")

print("Fetching latest release")
json = requests.get("https://api.github.com/repos/radareorg/cutter-deps/releases/latest").json()

release_url = json["assets"][0]["browser_download_url"]
for platform in platforms:
	release_url = release_url.replace(platform, "${PLATFORM}")

with open(fetch_deps_path) as f:
	fetch_deps = f.read()

md5 = {}
for platform in platforms:
	platform_url = release_url.replace("${PLATFORM}", platform)
	print(f"Getting MD5 for {platform_url}")

	curl = subprocess.Popen(["curl", "-fL", platform_url], stdout=subprocess.PIPE)
	md5sum = subprocess.run(md5sum_cmd, stdin=curl.stdout, capture_output=True, encoding="utf-8").stdout
	curl.wait()
	if curl.returncode != 0:
		print(f"Failed to download {platform_url}, skipping.")
		continue

	md5sum = re.fullmatch("([a-zA-Z0-9]+)( +-)?\n?", md5sum).group(1)

	print(f"MD5: {md5sum}")
	fetch_deps = re.sub(f"^{platform.upper()}_URL=.*$", f"{platform.upper()}_URL={platform_url}".replace("\\", r"\\"), fetch_deps, flags=re.MULTILINE)
	fetch_deps = re.sub(f"^{platform.upper()}_MD5=.*$", f"{platform.upper()}_MD5={md5sum}".replace("\\", r"\\"), fetch_deps, flags=re.MULTILINE)

with open(fetch_deps_path, "w") as f:
	f.write(fetch_deps)
