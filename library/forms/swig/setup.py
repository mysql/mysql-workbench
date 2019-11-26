
from distutils.core import setup, Extension

import subprocess
import sys

def pkgconfig(*packages, **kw):
    flag_map = {'-I': 'include_dirs', '-L': 'library_dirs', '-l': 'libraries'}
    for token in subprocess.getoutput("pkg-config --libs --cflags %s" % ' '.join(packages)).split():
        kw.setdefault(flag_map.get(token[:2]), []).append(token[2:])
    return kw


extra_sources = []

if sys.platform in ["linux2"]:
  options = pkgconfig("sigc++-2.0 gtkmm-2.4")

  options["include_dirs"].append("..")
  options["library_dirs"] = ["../.libs"] + options.get("library_dirs", [])
  options["libraries"].append("mforms")
  extra_sources.append("linux_stubs.cxx")
else:
  print("Platform not supported")
  sys.exit(1)

setup(name = "mforms",
      ext_modules = [Extension("_mforms", ["mforms_wrap.cxx"] + extra_sources, **options)]
     )
