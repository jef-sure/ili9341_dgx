import glob
import os
import re
from builtins import FileNotFoundError

spath = os.path.dirname(os.path.realpath(__file__))
all_fonts = [fon[len(spath) + 1:] for fon in  glob.glob(spath + '/fonts/*.c')]
all_headers = [re.sub(r"\.c$", ".h", f) for f in all_fonts]
root = spath + "/../../..";
used = set()
for cdir, subdirs, files in os.walk(root):
    for fname in files:
        if re.search(r"\.(c|cc|cpp|cxx)$", fname, re.IGNORECASE) is not None:
            fh = open(os.path.join(cdir, fname), mode='r')
            content = fh.read()
            fh.close()
            for header in all_headers:
                if re.search(r"^#include\s*\"" + header + "\"", content, re.MULTILINE) is not None:
                    used.add(re.sub(r"^fonts/", "", re.sub(r"\.h$", ".c", header)));
fontstxt = str.join("\n", ["src/fonts/" + uf for uf in sorted(used)])
fontsFile = '/fonts/fonts.txt'
try:
    fh = open(spath + fontsFile, 'r')
except FileNotFoundError:
    fh = None
if fh is not None:
    content = fh.read()
    fh.close()
if content != fontstxt:
    with open(spath + fontsFile, 'w') as cmt:
        cmt.write(fontstxt)

