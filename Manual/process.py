#!/usr/bin/env python3

# Automagic processing of the AGS manual, for converting to RestructuredText/Sphinx.

import re, sys

lines = []
handle = open('ags.tex', 'rt', encoding='utf-8')
for line in handle:
    lines.append(line.rstrip())
handle.close()


def header(start_file, kar, title, suffix):
    result = []
    if start_file:
        fname = title.lower().replace(' ', '-').replace('.', '_')
        result.append('.. ### Start file "' + fname + '"')
    result.append(title + suffix)
    result.append(kar * len(title))
    return result


def simple_replaces(line):
    if line.startswith('\\documentstyle['): return ['']
    if line.startswith('%\\input{psbox.tex}'): return ['']
    if line.startswith('\\newcommand{'): return ['']
    if line.startswith('\\parskip='): return ['']
    if line.startswith('\\parindent='): return ['']
    if line.startswith('\\backgroundcolour{'): return ['']
    if line.startswith('\\maketitle'): return ['']
    if line.startswith('\\makeindex'): return ['']
    if line.startswith('\\bibliographystyle'): return ['']
    if line.startswith('\\pagestyle{'): return ['']
    if line.startswith('\\pagenumbering'): return ['']
    if line.startswith('\\setheader{'): return ['']
    if line.startswith('\\setfooter{'): return ['']
    if line.startswith('\\begin{document}'): return ['']
    if line.startswith('\\end{document}'): return ['']

    if line.startswith('\\tableofcontents'): return ['','.. toctree::']

    if line == '\\fcol{red}{Example:}': return ['Example:']
    if line == '\\begin{itemize}': return ['']
    if line == '\\begin{itemize}\\itemsep=0pt': return ['']
    if line == '\\begin{itemize}\\itemsep=10pt': return ['']
    if line == '\\end{itemize}': return ['']
    if line.startswith('\\item '):
        line = '* ' + line[6:]

    m = re.compile('\\\\title{(.*)}%$').match(line)
    if m: return [':title: ' + m.group(1)]
    m = re.compile('\\\\author{(.*)}%$').match(line)
    if m: return [':author: ' + m.group(1)]

    m = re.compile('(.*)\\\\urlref{(.*)}{(.*)}$').match(line)
    if m:
        txt = m.group(3)
        if not txt.startswith('http:'): txt = 'http:' + txt
        return [m.group(1) + ':ref:`' + m.group(2) + ' <' + txt + '>`']

    m = re.compile('(.*)\\\\urlref{(.*)}{(.*)}\\.$').match(line)
    if m:
        txt = m.group(3)
        if not txt.startswith('http:'): txt = 'http:' + txt
        return [m.group(1) + ':ref:`' + m.group(2) + ' <' + txt + '>`']

    return [line]

def headers(line):
    m = re.compile('\\\\chapter{([^}]+)}(.*)%$').match(line)
    if m: return header(True, '#', m.group(1), m.group(2))
    m = re.compile('\\\\chapter\\*{([^}]+)}(.*)%$').match(line)
    if m: return header(True, '#', m.group(1), m.group(2))

    m = re.compile('\\\\section{([^}]+)}(.*)%?$').match(line)
    if m: return header(False, '=', m.group(1), m.group(2))
    m = re.compile('\\\\section\\*{([^}]+)}(.*)%?$').match(line)
    if m: return header(False, '=', m.group(1), m.group(2))

    m = re.compile('\\\\subsection{([^}]+)}(.*)%$').match(line)
    if m: return header(False, '-', m.group(1), m.group(2))
    m = re.compile('\\\\subsection\\*{([^}]+)}(.*)$').match(line)
    if m: return header(False, '-', m.group(1), m.group(2))

    m = re.compile('\\\\bf{([-:"\'A-Z0-9a-z ]+)}$').match(line)
    if m: return header(False, '.', m.group(1), '')
    m = re.compile('\\\\subsubsection\\*{([^}]+)}(.*)$').match(line)
    if m: return header(False, '.', m.group(1), m.group(2))

    return [line]

def label_replace(line):
    newlines = []
    m = re.compile('\\\\label{([^}]+)}').search(line)
    if m:
        newlines.append('.. _' + m.group(1) + ':')
        line = line[:m.start()] + line[m.end():]

    indices = []
    while True:
        m = re.compile('\\\\index{([- _:,A-Z()\\*#0-9a-z.]+)}').search(line)
        if not m:
            break
        indices.append('   ' + m.group(1).replace(',', ';'))
        line = line[:m.start()] + line[m.end():]

    if len(indices) > 0:
        newlines.append('')
        newlines.append('.. index::')
        newlines.extend(indices)

    if len(newlines) > 0:
        return [''] + newlines + ['', line]
    return [line]

def single_line_markup(line):
    if line.startswith('\\Large{') and line[-1] == '}':
        line = line[7:-1]
        return line
    if line.startswith('\\large{') and line.endswith('}\\hrule'):
        line = line[7:-7]
        return line
    if line.endswith('%'):
        line = line[:-1]

    while True:
        m = re.compile('\\\\bf{([^}]+)}').search(line)
        if not m:
            break
        content = m.group(1).strip()
        if content != '' and content[0] == '*': content = ' ' + content
        if content != '' and content[-1] == '*': content = content + ' '
        line = line[:m.start()] + '**' + content + '**' + line[m.end():]

    while True:
        m = re.compile('\\\\it{([^}]+)}').search(line)
        if not m:
            break
        content = m.group(1).strip()
        if content != '' and content[0] == '*': content = ' ' + content
        if content != '' and content[-1] == '*': content = content + ' '
        line = line[:m.start()] + '*' + content + '*' + line[m.end():]

    while True:
        m = re.compile('\\\\helpref{([^}]+)}{([^}]+)}').search(line)
        if not m:
            break
        line = line[:m.start()] + ':ref:`' + m.group(1) + ' <' + m.group(2) + '>`' + line[m.end():]

    while True:
        m = re.compile('\\\\helprefn{([^}]*)}{([^}]+)}').search(line)
        if not m:
            break
        line = line[:m.start()] + ':ref:`' + m.group(1) + ' <' + m.group(2) + '>`' + line[m.end():]

    while True:
        m = re.compile('\\\\verb\\|([^^]+)\\|').search(line)
        if not m:
            break
        line = line[:m.start()] + '``' + m.group(1) + '``' + line[m.end():]

    while True:
        m = re.compile('\\\\verb!([^^]+)!').search(line)
        if not m:
            break
        line = line[:m.start()] + '``' + m.group(1) + '``' + line[m.end():]

    while True:
        m = re.compile('\\\\verb\\^([^^]+)\\^').search(line)
        if not m:
            break
        line = line[:m.start()] + '``' + m.group(1) + '``' + line[m.end():]

    while True:
        m = re.compile('\\\\verb\\$([^$]+)\\$').search(line)
        if not m:
            break
        line = line[:m.start()] + '``' + m.group(1) + '``' + line[m.end():]

    return line

def add_figure(lines):
    newlines = []
    idx = 0
    while idx < len(lines):
        line = lines[idx]
        m = re.compile('LTSSimg src="([^"]*)" GTSS').match(line)
        if not m:
            newlines.append(line)
            idx = idx + 1
            continue
        else:
            newlines.append('')
            newlines.append('.. figure:: ' + m.group(1))
            idx = idx + 1
            if lines[idx] != '': # Caption.
                newlines.append('')
                newlines.append('   ' + lines[idx])
                idx = idx + 1
            continue

    return newlines

def replace_verbatim(lines):
    newlines = []
    inside = False
    for idx, line in enumerate(lines):
        if inside:
            assert not '\\begin{verbatim}' in line
            if re.compile(' *\\\\end{verbatim}').match(line):
                newlines.append('')
                inside = False
            else:
                newlines.append('   ' + line)
        else:
            if '\\end{verbatim}' in line:
                print("Spurious \\end{verbatim} at line " + str(idx+1))
                for x in newlines[-20:]:
                    print(x)
                assert False
            if re.compile(' *\\\\begin{verbatim}').match(line):
                newlines.append('')
                newlines.append('::')
                newlines.append('')
                inside = True
            else:
                newlines.append(line)
    assert not inside
    return newlines

def replace_enumerate(lines):
    newlines = []
    inside = False
    for idx, line in enumerate(lines):
        if inside:
            assert not line.startswith('\\begin{enumerate}')
            if line == '\\end{enumerate}':
                newlines.append('')
                inside = False
            else:
                if line.startswith('\\item '):
                    line = str(number) + '. ' + line[5:].lstrip()
                    number = number + 1
                else:
                    if not line.startswith('   '): line = '   ' + line.lstrip()
                newlines.append(line)
        else:
            if line.startswith('\\end{enumerate}'):
                print("Spurious \\end{enumerate} at line " + str(idx+1))
                for x in newlines[-20:]:
                    print(x)
                assert False
            if line == '\\begin{enumerate}':
                newlines.append('')
                inside = True
                number=1
            else:
                newlines.append(line)
    assert not inside
    return newlines

def merge_rows(lines):
    newlines = []
    for line in lines:
        if len(newlines) == 0:
            newlines.append(line)
            continue
        if re.compile(' *\\\\row{').match(line) or re.compile(' *\\\\end{tabular}').match(line):
            newlines.append(line)
            continue
        if re.compile(' *\\\\row{').match(newlines[-1]) and not re.compile('} *}$').search(newlines[-1][-1]):
            #print("Merging " + line)
            newlines[-1] = newlines[-1] + ' ' + line.lstrip()
            continue
        newlines.append(line)

    return newlines

def replace_tabular(lines):
    lines = merge_rows(lines)

    last_line = '\\end{tabular}'
    newlines = []
    inside = False
    table = []
    count = None
    row_pat = None
    for idx, line in enumerate(lines):
        if inside:
            assert not line.startswith('\\begin{tabular}')
            if line == last_line:

                # Output table
                #sizes = [0, 0]
                for row in table:
                    for idx, val in enumerate(row):
                        sizes[idx] = max(sizes[idx], len(val))
                newlines.append('')
                newlines.append('  '.join('='*x for x in sizes))
                for ridx, row in enumerate(table):
                    cols = []
                    for cidx, col in enumerate(count):
                        val = row[cidx]
                        t = sizes[cidx] - len(val)
                        if col == 'l':
                            val = val + ' '*t
                        elif col == 'c':
                            t2 = t // 2
                            t = t - t2
                            val = ' '*t + val + ' '*t2
                        else:
                            val = ' '*t + val
                        cols.append(val)
                    newlines.append('  '.join(cols))
                    if ridx == 0:
                        newlines.append('  '.join('='*x for x in sizes))
                newlines.append('  '.join('='*x for x in sizes))
                newlines.append('')

                inside = False
                table = []
            else:
                m = row_pat.match(line)
                if m:
                    table.append(tuple(m.group(i).strip() for i in range(1, len(sizes)+1)))
                else:
                    print(idx, repr(line))
                    assert False
        else:
            if line.startswith(last_line):
                print('Spurious ' + last_line + ' at line ' + str(idx+1))
                for x in newlines[-20:]:
                    print(x)
                assert False
            if not line.startswith('\\begin{tabular}'):
                newlines.append(line)
            else:
                count = None
                m = re.compile('\\\\begin{tabular}{\\|([clr])\\|([clr])\\|}$').match(line)
                if m:
                    count = [m.group(1), m.group(2)]
                    sizes = [0, 0]
                    row_pat = re.compile('\\\\row{ *{(.*)} *& *{(.*)} *}$')
                m = re.compile('\\\\begin{tabular}{\\|([clr])@{ }([clr])\\|}$').match(line)
                if m:
                    count = [m.group(1), m.group(2)]
                    sizes = [0, 0]
                    row_pat = re.compile('\\\\row{ *{(.*)} *& *{(.*)} *}$')
                m = re.compile('\\\\begin{tabular}{\\|([clr])\\|([clr])\\|([clr])\\|}$').match(line)
                if m:
                    count = [m.group(1), m.group(2), m.group(3)]
                    sizes = [0, 0, 0]
                    row_pat = re.compile('\\\\row{ *{(.*)} *& *{(.*)} *& *{(.*)} *}$')

                assert count is not None
                inside = True

    assert not inside
    return newlines

def replace_multiline_itbf(itbf, repl, lines):
    newlines = []
    i = 0
    start = '\\' + itbf + '{'
    while i < len(lines):
        if start not in lines[i]:
            newlines.append(lines[i])
            i = i + 1
        else:
            j = lines[i].find(start)
            if '}' in lines[i][j:]:
                print("HUH, unexpected } found " + lines[i])
                assert False
            k = 1
            while '}' not in lines[i + k]:
                k = k + 1
                if k >= 20:
                    print("Section " + start + " too long near " + lines[i])
            n = lines[i+k].find('}')

            newlines.append(lines[i][:j] + repl + lines[i][j+4:])
            p = 1
            while p < k:
                newlines.append(lines[i+p])
                p = p + 1
            newlines.append(lines[i+k][:n] + repl + lines[i+k][n+1:])
            i = i + k + 1

            #for x in lines[i-20:i]: sys.stdout.write(x + '\n')
            #print("====")
            #for x in newlines[-20:]: sys.stdout.write(x + '\n')

            #sys.exit(0)


    return newlines

def replace_double_colon(lines):
    newlines = []
    for line in lines:
        if line != '::':
            newlines.append(line)
            continue
        idx = len(newlines)-1
        while idx >= 0 and newlines[idx] == '':
            idx = idx - 1
        if idx >= 0:
            last = newlines[idx]
            if last.endswith('::'):
                if idx+1 < len(newlines): newlines = newlines[:idx+1]
                continue
            if last.endswith(':'):
                if idx+1 < len(newlines): newlines = newlines[:idx+1]
                newlines[-1] = newlines[-1] + ':'
                continue
        newlines.append(line)

    return newlines


def drop_multi_empty(lines, allowed):
    newlines = []
    empty_count = 0
    for line in lines:
        line = line.rstrip()
        if line == '':
            if empty_count > allowed: continue
            empty_count = empty_count + 1
        else:
            empty_count = 0
        newlines.append(line)

    return newlines

def split_files(lines):
    names = []
    files = {}
    name = None
    for line in lines:
        m = re.compile('\\.\\. ### Start file "(.*)"$').match(line)
        if m:
            name = m.group(1)
        else:
            out = files.get(name)
            if out is None:
                names.append(name)
                out = []
                files[name] = out
            out.append(line)

    del_files = []
    for name, lines in files.items():
        while len(lines) > 0 and lines[-1] == '': del lines[-1]
        while len(lines) > 0 and lines[0] == '': del lines[0]
        if len(lines) == 0: del_files.append(name)

    if len(del_files) > 0:
        for name in del_files:
            del files[name]
        names = [nm for nm in names if nm not in del_files]

    return files, names


bad_lines = 0

lines = replace_enumerate(lines)

newlines = []
for line in lines:
    newlines.extend(simple_replaces(line))
lines = newlines

newlines = []
for line in lines:
    newlines.extend(headers(line))
lines = newlines

newlines = []
for line in lines:
    newlines.extend(label_replace(line))
lines = newlines

lines = [single_line_markup(line) for line in lines]

# Multi-line stuff.
lines = replace_verbatim(lines)
lines = replace_multiline_itbf('it', '*', lines)
lines = replace_multiline_itbf('bf', '**', lines)
lines = replace_tabular(lines)
lines = add_figure(lines)
lines = replace_double_colon(lines)
lines = drop_multi_empty(lines, 1)

#for index, line in enumerate(lines):
#    if '\\' in line:
#        print("{:5d}: {}".format(index, line))
#        bad_lines = bad_lines + 1
#        if bad_lines > 40:
#            break
#    elif True and False:
#        print("     : {}".format(line))
#    print(line)

files, names = split_files(lines)
for name, lines in files.items():
    if name is None: name = "ags"
    handle = open(name + ".rst", "wt", encoding="utf-8")
    for line in lines:
        if line == '.. toctree::':
            table = ['   :maxdepth: 2\n', '\n']
            for nm in names:
                if nm is None: nm = "ags"
                if nm == name: continue
                table.append("   " + nm + "\n")
            line = line + '\n' + "".join(table) # The \n below will add an empty line.
        handle.write(line)
        handle.write('\n')
    handle.close()
