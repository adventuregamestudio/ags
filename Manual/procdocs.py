#!/usr/bin/env python

import glob
import sys

# Format generated .htm to have correct syntax and additional styling.

STYLE = """\
<style type=\"text/css\"><!--
  body {
    font-family: Verdana;
    font-size: 10pt;
  }
  td {
    font-family: Verdana;
    font-size: 10pt;
  }
  a {
    font-weight: bold
  }
--></style>"""

def process_file(filename):
    print "Processing {0}...".format(filename)

    with file(filename, 'rb') as inp:
        data = inp.read()

    data = data.replace("ILBRK", "<br>")
    data = data.replace("GTSS", ">")
    data = data.replace("LTSS", "<")

    if "</title></head>" in data or "</TITLE></HEAD>" in data:
        insert_index = data.find("</head>")
        if insert_index < 0:
            insert_index = data.find("</HEAD>")
        if insert_index < 0:
            raise Exception("Could not find end of <head> block.")
        data = data[:insert_index] + STYLE + data[insert_index:]

    with file(filename, 'wb') as oup:
        oup.write(data)

def main():
    htm_filenames = glob.glob("*.htm")

    if not htm_filenames:
        print "No *.htm files in current directory!"
        sys.exit(1)

    for filename in htm_filenames:
        try:
            process_file(filename)
        except Exception, e:
            print("{0}: {1}".format(filename, e.message))
            sys.exit(1)

if __name__ == "__main__":
    main()
