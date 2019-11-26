
import os
import sys
import codecs
import datetime
import time
import re

rootdir = ""
create = False
simulation = True # This will create files with *.headers_tmp instead of replacing the real files
fixCommercial = False

#
# Usage:
#   python file_headers.py [options] [path]
#
#   Options:
#      --create      creates missing headers
#      --real        Change the original files
#      --commercial  deals with commercial files
#

for arg in sys.argv[1:]:
    if arg == '--create':
        create = True
    elif arg == '--real':
        simulation = False
    elif arg == '--commercial':
        fixCommercial = True
    else:
        rootdir = arg

cppTemplate=None
pythonTemplate=None
dirCount = 0
updatedFiles=[]
ignoredFiles=[]
commercialFiles=[]
headerAdded=[]
eolChanged=[]
bomRemoved=[]
unchangedFiles=[]
parsableFiles=['h', 'cpp', 'py', 'm', 'mm', 'cs']
ignoreList = []
comercialList = []
failedFiles = []
copyrightStrings = ['* Copyright (c) ', '* Copyright(c) ', '*  Copyright ', 'Copyright ', '*  Copyright ', '//  Copyright ']

def getFileContents(filename):
    data=None
    with open(filename, 'r') as f:
        data = f.read()
    f.closed
    return data

def writeFile(filename, header, contents):
    if simulation:
        filename = filename + ".headers_tmp"
    with open(filename, 'w') as f:
        f.write(header)
        f.write('\n')
        f.write(contents)
    f.closed

class Parser:
    def __init__(self):
        self.startLine = None
        self.endLine = None
        self.creationYear = None

    def __str__(self):
        return "[%s:%s][%s]" % (self.startLine, self.endLine, self.creationYear)

    @property
    def hasHeader(self):
        return self.startLine is not None and self.endLine is not None and self.creationYear is not None

    @property
    def nothingFound(self):
        return self.startLine is None and self.endLine is None and self.creationYear is None

    @property
    def onlyEnd(self):
        return self.startLine is None and self.endLine is not None and self.creationYear is None
      
class CParser(Parser):
    def __init__(self):
        self.startLine = None
        self.creationYear = None
        self.updateYear = None
        self.endLine = None
        
    def getHeader(self, currentYear):
        #print('creation year: %s, current year: %s\n%s' % (self.creationYear, currentYear, cppTemplate))
        return cppTemplate.replace('<ORIG_YEAR>', str(self.creationYear)).replace('<CURRENT_YEAR>', str(currentYear))

    def getCommercialHeader(self, currentYear):
        return cppCommercialTemplate.replace('<ORIG_YEAR>', str(self.creationYear)).replace('<CURRENT_YEAR>', str(currentYear))

    def detectHeader(self, contentsArray, filename):
        inComment = False
        inlineComment = False
        
        max = 25
        if len(contentsArray) < max:
            max = len(contentsArray)
        
        for line in range(0, max):
            text = contentsArray[line].strip()
            if text.startswith('/*'):
                inComment = True
                self.startLine = line
                text = text[2:].strip()
            elif text.find('/*') > -1:
                print(("  BOM problem? %s" % filename))
                
            if text.endswith('*/'):
                inComment = False
                self.endLine = line
                text = text[:-2].strip()
            if not inlineComment and text.startswith('//'):
                self.startLine = line
                inlineComment = True
            if inlineComment and not text.startswith('//'):
                    self.endLine = line
                    inlineComment = False
            
            for str in copyrightStrings:
                if (inComment or inlineComment) and text.startswith(str):
                    years = list(map(int, re.findall(r'\d+', text)))
                    self.creationYear = years[0]
                    self.updateYear = years[-1]
            
            if self.hasHeader:
                break

class PythonParser(Parser):
    def __init__(self):
        self.shebang = None
        self.startLine = None
        self.creationYear = None
        self.updateYear = None
        self.endLine = None
        
    def getHeader(self, currentYear):
        return pythonTemplate.replace('<ORIG_YEAR>', str(self.creationYear)).replace('<CURRENT_YEAR>', str(currentYear))

    def getCommercialHeader(self, currentYear):
        return pythonCommercialTemplate.replace('<ORIG_YEAR>', str(self.creationYear)).replace('<CURRENT_YEAR>', str(currentYear))

    def detectHeader(self, contentsArray, filename):
        max = 25
        if len(contentsArray) < max:
            max = len(contentsArray)
        
        for line in range(0, max):
            text = contentsArray[line].strip()
            if text.startswith('#!'):
                self.shebang = text
                if self.startLine == None:
                    self.startLine = line
            elif text.startswith('# Copyright (c) '):
                if self.startLine == None:
                    self.startLine = line
                
                years = list(map(int, re.findall(r'\d+', text)))
                self.creationYear = years[0]
                self.updateYear = years[-1]
                    
            elif line == 0 and text.find('# Copyright (c) ') > -1:
                print(("    BOM problem?  %s" % filename))
            elif self.startLine is not None and self.creationYear is not None and not text.startswith('#'):
                self.endLine = line
                break

def commercialHeader(filename):
    for item in commercialList:
        if item.startswith('*') and item.endswith('*'):
            if filename.find(item[1:-1]) > -1:
                return True
        elif item.startswith('*'):
              if filename.endswith(item[1:]):
                  return True
        elif item.endswith('*'):
              if filename.startswith(item[:-1]):
                  return True
        elif filename.startswith(item):
            return True
    return False
  
def ignoreFile(filename):
    for ignore in ignoreList:
        if ignore.startswith('*') and ignore.endswith('*'):
            if filename.find(ignore[1:-1]) > -1:
                return True
        elif ignore.startswith('*'):
              if filename.endswith(ignore[1:]):
                  return True
        elif ignore.endswith('*'):
              if filename.startswith(ignore[:-1]):
                  return True
        elif filename.startswith(ignore):
            return True
    return False

def browseDirectory(directory):
    global dirCount
    global rootdir

    folder = None
    subs = None
    files = None
    
    for folder, subs, files in os.walk(directory):
        dirCount += 1
        for filename in files:
            fileExtension = os.path.splitext(filename)[1][1:]
            if not fileExtension in parsableFiles:
                continue
            
            fullFilename = os.path.join(folder, filename)
            
            if ignoreFile(fullFilename):
                ignoredFiles.append(fullFilename)
                continue
            
            if not os.path.exists(fullFilename):
                print(("File does not exist: %s" % fullFilename))
                exit(1)
            
            fileContents = getFileContents(fullFilename)
            
            needsUpdate = False
            
            if fileContents.startswith(codecs.BOM_UTF8):
                fileContents = fileContents.decode('utf-8-sig')
                bomRemoved.append(fullFilename)
                needsUpdate = True
            
            if not fileContents == fileContents.replace('\r\n', '\n'):
                fileContents = fileContents.replace('\r\n', '\n')
                eolChanged.append(fullFilename)
                needsUpdate = True
                
            modification_year = time.strftime('%Y', time.localtime(os.path.getmtime(fullFilename)))

            contentsArray = fileContents.split('\n')
            newHeader = None
            parser = None

            if fileExtension in ['h', 'cpp', 'm', 'mm', 'cs']:
                parser = CParser()
            elif fileExtension in ['py']:
                parser = PythonParser()

            parser.detectHeader(contentsArray, fullFilename)
            
            if not parser.hasHeader:
                # Header does not exist...we need to create one
                if create:
                    headerAdded.append([parser.startLine, parser.endLine, parser.creationYear, fullFilename])
                    #headerAdded.append(fullFilename)
                    #print("Adding header to file: %s" % fullFilename)
                    parser.startLine = 0
                    parser.endLine = 0
                    parser.creationYear = 2009
                    parser.updateYear = 2009
                    needsUpdate = True
                else:
                    failedFiles.append([parser.startLine, parser.endLine, parser.creationYear, fullFilename])
                    #failedFiles.append(fullFilename)
                    #continue
            
            # Check if the modification date is superior then the year set in the copyright
            if int(parser.updateYear) < int(modification_year):
                needsUpdate = True
            
            if parser.endLine > 0:
                # Full header was detected
                for line in range(parser.startLine, parser.endLine + 1):
                    del contentsArray[parser.startLine]
                    
                while len(contentsArray) > parser.startLine and contentsArray[parser.startLine].strip() == '':
                    del contentsArray[parser.startLine]

            newHeader = parser.getHeader(modification_year)
            if fixCommercial and commercialHeader(fullFilename):
                newHeader = parser.getCommercialHeader(modification_year)
                if needsUpdate:
                    commercialFiles.append(fullFilename)
            
            # Add the shebang before the copyright
            if fileExtension in ['py'] and parser.shebang:
                newHeader = parser.shebang + '\n' + newHeader
            
            # Update the file only if it needs to
            if needsUpdate:
                writeFile(fullFilename, newHeader,  '\n'.join(contentsArray))
                updatedFiles.append(fullFilename)
            else:
                unchangedFiles.append(fullFilename)
            
            


cppTemplate = getFileContents('copyright_cpp_template')
cppCommercialTemplate = getFileContents('copyright_cpp_commercial_template')
pythonTemplate = getFileContents('copyright_python_template')
pythonCommercialTemplate = getFileContents('copyright_python_commercial_template')
ignoreList = getFileContents('ignore').split('\n')

ignoreList = [_f for _f in ignoreList if _f]
for index in range(0, len(ignoreList)):
    if not ignoreList[index].startswith('*'):
        ignoreList[index] = os.path.join(os.path.realpath(rootdir), ignoreList[index])

commercialList = getFileContents('removals').split('\n')

commercialList = [_f for _f in commercialList if _f]
for index in range(0, len(commercialList)):
    if not commercialList[index].startswith('*'):
        commercialList[index] = os.path.join(os.path.realpath(rootdir), commercialList[index])


def listDiff(first, second):
    second = set(second)
    return [item for item in first if item not in second]

browseDirectory(os.path.realpath(rootdir))

print(('Parsed %d dirs and %d files' % (dirCount, len(updatedFiles))))

print(("\n\nAll updated files (%d):" % len(updatedFiles)))
for file in updatedFiles:
    print(file)

print(("\n\nFailed files (%d):" % len(failedFiles)))
for file in failedFiles:
    print(file)
    
print(("\n\nCommercial headers (%d):" % len(commercialFiles)))
for file in commercialFiles:
    print(file)

print(("\n\nAdded headers (%d):" % len(headerAdded)))
for file in headerAdded:
    print((file[3]))

print(("\n\nEOL Changed (%d):" % len(eolChanged)))
for file in eolChanged:
    print(file)

print(("\n\nBOM removed (%d):" % len(bomRemoved)))
for file in bomRemoved:
    print(file)

#print("\n\nUnchanged (%d):" % len(unchangedFiles))
#for file in unchangedFiles:
    #print(file)

