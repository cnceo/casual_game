#!/usr/bin/python3

#-*-coding:utf-8-*-


import os
import re


class LogFile(object):
    def __init__(self, path):
        self.name = path
        self.lines = []
        f = open(path)
        for line in f:
            self.lines.append(line)

    def grepLines(self, *keywords):
        pass

    def egrepLines(self, regular):
        pass


class Logs(object):
    def __init__(self, directory):
        self.files = []
        for filename in os.listdir(directory):
            path = os.path.join(directory, filename)
            if os.path.isfile(path):
                self.files.append(LogFile(path))
        pass

    def grepQingLong(self):
        for f in self.files:
            for l in f.lines:
                if l.find('deal deck') == -1:
                    continue
                ret = re.search('cards=\[.+\]', l)
                if not ret:
                    continue
                begin, end = ret.span()
                cardstrs = l[begin + 7, end - 2].split(',')
                #print(cardstrs)
                ranks = [(int(cardstr) - 1) % 13 for cardstr in cardstrs]
                suits = [(int(cardstr) - 1) // 13 for cardstr in cardstrs]

                isStright = cards == [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
                isFlush   = len(set(suits)) == 1

                if isStright and isFlush:
                    print("发牌, 同花大顺")
                    print(l)
                elif isStright:
                    print("发牌, 大顺")
                    print(l)
                elif isFlush:
                    print("发牌, 同花")
                    print(l)








logs = Logs("../log")
logs.grepQingLong()



