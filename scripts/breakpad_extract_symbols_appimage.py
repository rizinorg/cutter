
import sys
import os
import subprocess
import re
import atexit

if len(sys.argv) != 3:
	print(f"usage: {sys.argv[0]} [Cutter.AppImage] [symbols dir]")
	exit(1)

root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

def store_syms(syms, syms_dir):
	m = re.match(b"MODULE ([^ ]+) ([^ ]+) ([^ ]+) (.+)\n.*", syms)
	if m is None:
		print("Invalid dump_syms output")
		return
	
	(modos, modarch, modhash, modname) = m.groups()
	modname = modname.decode("utf-8")
	modhash = modhash.decode("utf-8")
	symdir = os.path.join(syms_dir, modname, modhash)
	symfile = f"{modname}.sym"
	os.makedirs(symdir)
	symfile_path = os.path.join(symdir, symfile)
	with open(symfile_path, "wb") as f:
		f.write(syms)

	print(symfile_path)

def dump_syms(binary, syms_dir):
	dump_syms_exec = os.path.join(root, "breakpad/src/tools/linux/dump_syms/dump_syms")
	syms = subprocess.run([dump_syms_exec, binary], capture_output=True).stdout
	store_syms(syms, syms_dir)

appimage = sys.argv[1]
syms_dst = sys.argv[2]

# stdbuf workaround is needed before https://github.com/AppImage/AppImageKit/commit/e827baa719f5444aeef7202fe1f71c97d4200dde
appimage_p = subprocess.Popen(["stdbuf", "-oL", appimage, "--appimage-mount"], stdout=subprocess.PIPE)
def kill_appimage():
	appimage_p.kill()
atexit.register(kill_appimage)
mount_dir = appimage_p.stdout.readline().strip().decode("utf-8")

binaries = [ os.path.join(mount_dir, "usr/bin/Cutter") ]
for f in os.scandir(os.path.join(mount_dir, "usr/lib")):
	if f.is_dir() or f.is_symlink():
		continue
	binaries.append(f.path)

for b in binaries:
	dump_syms(b, syms_dst)

