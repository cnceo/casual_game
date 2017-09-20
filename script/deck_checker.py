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
        self.files = {}
        for filename in os.listdir(directory):
            path = os.path.join(directory, filename)
            if os.path.isfile(path):
                self.files[path] = LogFile(path)
        pass

    def grepQingLong(self):
        pass



logs = Logs("../log")
for filename in logs.files:
    print(filename)



