#!/usr/bin/env python
"""
Summarize whats in the repro logs.
Run: script -c "./runTest outputs/*/crashes/id*"; ./summary.py |sort -u
"""

import sys, re

known = [
    'ended with status 9', # ignore all timeouts
    'ended with status 0', # ignore all unreproducibles
]

def isKnown(x) :
    return any(re.search(pat, x) for pat in known)

def proc(ls) :
    if not ls :
        return
    keep = ''
    for l in ls :
        if l.startswith('call') :
            nr = int(l.split(' ')[-1])
            keep += callnr.get(nr, '???') + ' '
        if (l.startswith('call') or
            l.startswith('panic') or
            l.startswith('test ended')) :
            keep += l + ' '
    if keep and (not SKIPKNOWN or not isKnown(keep)) :
        print keep

callnr = dict()
for l in file('../usr/src/sys/sys/syscall.h') :
    if l.startswith('#define	SYS_') :
        ws = l.split('\t')
        nm = ws[1][4:]
        num = int(ws[2])
        callnr[num] = nm

SKIPKNOWN = 1

f = file('typescript','r')

r = []
for l in f :
    l = l.rstrip()
    if l.startswith('Input from') :
        proc(r)
        r = []
    r.append(l)
proc(r)
