Ultra short manual for Sphinx:

- Download and install Sphinx (http://sphinx-doc.org)

- Since GIT cannot save empty directories, this directory should also have
  '_static' and '_templates' directories here. (Although you may be able to do
  without them if you don't use them.)

- Unix users can use the Makefile, Windows users can use the make.bat file.

  - "make clean" cleans up the generated data.
  - "make html" build the HTML documentation.
  - "make htmlhelp" builds HTML files for Windows HTML help.
    Once built, you need to compile them to Help files at Windows, see also
    http://ridingpython.blogspot.nl/2011/09/how-to-create-windows-html-help-chm.html

- I just did a rough check. Likely, there are still some issues. For example,
  the scripting chapter is way too long. Also, the tutorial .htm files could also
  be converted, I think. Last but not least, you can do lots of fine-tuning and
  restyling of Sphinx HTML output. I didn't do any of that either.

- I kept all existing files here to not disturb the currently working solution.
  Obviously, most of them are obsolete if you use Sphinx instead.


- The 'process.py' file is a custom Python 3 script to convert the ags.tex file
  to the *.rst files. It works almost completely, the final changes are listed in
  all_changes.patch .
