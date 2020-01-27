#!/usr/bin/python3
import glob
import os
import sys

def gen_grid(path):
    files = sorted(list(glob.glob(os.path.join(path, '*.png'))))
    for infile in files:
        print('<img src="%s"><figcaption>%s</figcaption>' % (infile, infile))


gen_grid(sys.argv[1] if len(sys.argv) > 1 else '')