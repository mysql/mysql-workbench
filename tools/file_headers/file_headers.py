
import os
import sys

rootdir = ""
force = False
simulation = True # This will create files with *.headers_tmp instead of replacing the real files
fixCommercial = False

for arg in sys.argv[1:]:
    if arg == '--force':
        force = True
    elif arg == '--real':
        simulation = False
    elif arg == '--commercial':
        fixCommercial = True
    else:
        rootdir = arg

cppTemplate=None
pythonTemplate=None
dirCount = 0
usedFiles=[]
ignoredFiles=[]
commercialFiles=[]
headerAdded=[]
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
    def getHeader(self, currentYear):
        return cppTemplate.replace('<ORIG_YEAR>', self.creationYear).replace('<CURRENT_YEAR>', currentYear)

    def getCommercialHeader(self, currentYear):
        return cppCommercialTemplate.replace('<ORIG_YEAR>', self.creationYear).replace('<CURRENT_YEAR>', currentYear)

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
                print("  BOM problem? %s" % filename)
                
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
                    text = text.replace(str, '').replace(',', '')
                    words = text.split()
                    self.creationYear = words[0]
            
            if self.hasHeader:
                break

        #if self.endLine and len(contentsArray) > self.endLine + 1 and contentsArray[self.endLine + 1].strip() == '':
            #self.endLine = self.endLine + 1

class PythonParser(Parser):
    def getHeader(self, currentYear):
        return pythonTemplate.replace('<ORIG_YEAR>', self.creationYear).replace('<CURRENT_YEAR>', currentYear)

    def getCommercialHeader(self, currentYear):
        return pythonCommercialTemplate.replace('<ORIG_YEAR>', self.creationYear).replace('<CURRENT_YEAR>', currentYear)

    def detectHeader(self, contentsArray, filename):
        max = 25
        if len(contentsArray) < max:
            max = len(contentsArray)
        
        for line in range(0, max):
            text = contentsArray[line].strip()
            #print(text)
            if text.startswith('# Copyright (c) '):
                #print("found header")
                self.startLine = line
                text = text.replace('# Copyright (c) ', '').replace(',', '')
                words = text.split()
                self.creationYear = words[0]
            elif line == 0 and text.find('# Copyright (c) ') > -1:
                print("    BOM problem?  %s" % filename)
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
                print("File does not exist: %s" % fullFilename)
                exit(1)
            
            fileContents = getFileContents(fullFilename)

            contentsArray = fileContents.split('\n')
            newHeader = None
            parser = None

            if fileExtension in ['h', 'cpp', 'm', 'mm', 'cs']:
                parser = CParser()
            elif fileExtension in ['py']:
                parser = PythonParser()

            #if filename != 'os_utils.py':
                #continue

            parser.detectHeader(contentsArray, fullFilename)
            
            if not parser.hasHeader:
                
                if force:
                    headerAdded.append([parser.startLine, parser.endLine, parser.creationYear, fullFilename])
                    #headerAdded.append(fullFilename)
                    #print("Adding header to file: %s" % fullFilename)
                    parser.startLine = 0
                    parser.endLine = 0
                    parser.creationYear = "2009"
                else:
                    failedFiles.append([parser.startLine, parser.endLine, parser.creationYear, fullFilename])
                    #failedFiles.append(fullFilename)
                    continue
            #print(parser)
            
            if parser.endLine > 0:
                for line in range(parser.startLine, parser.endLine + 1):
                    del contentsArray[parser.startLine]
                    
                while len(contentsArray) > parser.startLine and contentsArray[parser.startLine].strip() == '':
                    del contentsArray[parser.startLine]

            #print("%s - %s" % (filename, str(parser)))
            newHeader = parser.getHeader('2018')
            if fixCommercial and commercialHeader(fullFilename):
                newHeader = parser.getCommercialHeader('2018')
                commercialFiles.append(fullFilename)
            
            writeFile(fullFilename, newHeader,  '\n'.join(contentsArray))
            
            usedFiles.append(fullFilename)
            


cppTemplate = getFileContents('copyright_cpp_template')
cppCommercialTemplate = getFileContents('copyright_cpp_commercial_template')
pythonTemplate = getFileContents('copyright_python_template')
pythonCommercialTemplate = getFileContents('copyright_python_commercial_template')
ignoreList = getFileContents('ignore').split('\n')

ignoreList = filter(None, ignoreList)
for index in range(0, len(ignoreList)):
    if not ignoreList[index].startswith('*'):
        ignoreList[index] = os.path.join(os.path.realpath(rootdir), ignoreList[index])

commercialList = getFileContents('removals').split('\n')

commercialList = filter(None, commercialList)
for index in range(0, len(commercialList)):
    if not commercialList[index].startswith('*'):
        commercialList[index] = os.path.join(os.path.realpath(rootdir), commercialList[index])


browseDirectory(os.path.realpath(rootdir))

print('Parsed %d dirs and %d files' % (dirCount, len(usedFiles)))

print("\n\nFailed files (%d):" % len(failedFiles))
for file in failedFiles:
    print(file)
    
print("\n\nCommercial headers (%d):" % len(commercialFiles))
for file in commercialFiles:
    print(file)

print("\n\nAdded headers (%d):" % len(headerAdded))
for file in headerAdded:
    print(file[3])

