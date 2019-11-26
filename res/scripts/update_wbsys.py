#!/usr/bin/python

import subprocess
import os
import sys
import shutil
import xml.dom.minidom
import xml.dom
import uuid
import stat

WORKING_DIR = os.getcwd()

os.chdir(os.getcwd() + "/../..")

banned_files = ('README.md', 'LICENSE', '.git', 'COPYING', 'NEWS.md')
files_to_keep = ('sys_reports.js')

PROJECT_DIR = os.getcwd() + "/"
SCRIPT_DEPLOYMENT_DIR = PROJECT_DIR + "res/scripts/sys/"
MSI_FRAGMENT_FILE = PROJECT_DIR + "build/msi/source/mysql_workbench_fragment.xml"
CMAKE_FILE = PROJECT_DIR + "res/scripts/CMakeLists.txt"

clone_repo = False

def print_help():
    print('Usage: python update_wbsys.py [Options]')
    print('Options:')
    print('  --clone        Force to clone the repository')
    print('  --help         Prints this help')

if len(sys.argv) == 1:
    clone_repo = False
elif len(sys.argv) > 1:
    if sys.argv[1] == '--clone':
        clone_repo = True
    elif sys.argv[1] == '--help':
        print_help()
        exit(0)
    else:
        print(('Unknown option: ' + sys.argv[1]))
        print_help()
        exit(0)
else:
    print_help()
    exit(0)


print(("Working dir  : " + WORKING_DIR))
print(("Project dir  : " + PROJECT_DIR))
print(("MSI Frag file: " + MSI_FRAGMENT_FILE))
print(("CMake file   : " + CMAKE_FILE))
print(("Clone repo   : " + str(clone_repo)))

# This class searches a MSI XML fragment file for the wbsys 
# files, so that they get installed along with WB. The fragment
# file is updated with the processed wbsys files
class XMLFileManager:
  
    # The constructor reads the fragment file and finds the right
    # component element that will contain the wbsys files
        
    def __init__(self):
        self.directories = dict()
        self.files = dict()
        self.fileCount = 0
        
        self.xml_document = xml.dom.minidom.parse(MSI_FRAGMENT_FILE)
    
        self.root = self.xml_document.documentElement
        directory_elements = self.root.getElementsByTagName("Directory")
  
        for directory_element in directory_elements:
    
            if directory_element.getAttribute("Id") == "sys_dir":
                component = directory_element.getElementsByTagName("Component").item(0)
                
                indent = directory_element.parentNode.firstChild

                # Reuse the base node, so that we have the same ID
                while component.firstChild != None:
                    component.removeChild(component.firstChild)
                    
                while directory_element.firstChild != None:
                    directory_element.removeChild(directory_element.firstChild)
                
                directory_element.appendChild(indent.cloneNode(False))
                
                self.appendNodeChild(component, directory_element)
                
                self.directories['sys'] = directory_element
                
                break

    
    def appendNodeChild(self, child, parent):
      
      parentIndent = parent.lastChild
      
      childIndent = parentIndent.cloneNode(False)
      childIndent.nodeValue = childIndent.nodeValue + '  '
      
      parent.insertBefore(childIndent.cloneNode(False), parent.lastChild)
      parent.insertBefore(child, parent.lastChild)

      ## Don't create child indentation
      if child.tagName == 'File':
        return

      if child.hasChildNodes() == False:
        child.appendChild(childIndent.cloneNode(False))
      elif child.lastChild.nodeType != child.TEXT_NODE:
        child.appendChild(childIndent.cloneNode(False))
      else:
        print('Node already as indentation')
        
        
    def add_directory(self, directory):
        
        parent_dir = os.path.dirname(directory)
        current_dir = os.path.basename(directory)
        
        #parent_node = self.directories[parent_dir]
        entryId = directory.replace('/', '_')
        
        newEntry = self.xml_document.createElement("Directory")
        newEntry.setAttribute("Id", entryId + '_dir')
        newEntry.setAttribute("Name", current_dir)

        self.appendNodeChild(newEntry, self.directories[parent_dir])        
        self.directories[directory] = newEntry
      
      
    def add_file(self, directory, filename):
        self.fileCount = self.fileCount + 1
        
        newFile = self.xml_document.createElement("File")
        
        newFile.setAttribute('Id', 'wbsys' + '%03d' % self.fileCount)
        newFile.setAttribute('Name', filename)
        
        component = self.directories[directory].getElementsByTagName("Component").item(0)
        
        if component == None:
            component = self.xml_document.createElement("Component")
            component.setAttribute('DiskId', '1')
            component.setAttribute('Guid', str(uuid.uuid1()).upper())
            component.setAttribute('Id', directory.replace('/', '_'))
            component.setAttribute('Location', 'either')
            self.appendNodeChild(component, self.directories[directory])
        
        self.appendNodeChild(newFile, component)
    
    # Overwrites the original XML file
    def write_file(self):

        self.directories['sys'].normalize() 
        for node in self.directories:
          self.directories[node] = self.directories[node].normalize()

        text = self.xml_document.toxml()

        f = open(MSI_FRAGMENT_FILE, mode='w')
        f.write(text.replace("\r\n", "\n"))
        f.close()
           



# This class deals with a CMakeLists.txt file in order
# to setup the proper wbsys files to install.
class CMakeFileManager:
    file_lines_before = None
    file_lines = None
    file_lines_after = None
    state = 0
    
    # The constructor read the file into line arrays, so
    # They can be writen again properly. The automates files
    # are always discarted, and new ones are set during the
    # process.
    def __init__(self):
        with open(CMAKE_FILE) as f:
            self.file_lines_before = []
            self.file_lines_after = []
            self.file_lines = dict()
            
            for line in f:
                if self.state == 0:
                    self.file_lines_before.append(line)
                    if line.startswith("# SYS_BEGIN"):
                        self.state = 1
                elif self.state == 1:
                    if line.startswith("# SYS_END"):
                        self.file_lines_after.append(line)
                        self.state = 2
                else:
                    self.file_lines_after.append(line)

    def add_file(self, filename):
        if not os.path.dirname(filename) in self.file_lines:
            self.file_lines[os.path.dirname(filename)] = []
            
        self.file_lines[os.path.dirname(filename)].append(filename)
        
    def write_file(self):
        # Overwrite the original file
        f = open(CMAKE_FILE, mode='w')
        f.writelines(self.file_lines_before)
        
        for cmake_dir in self.file_lines:
            file_tag = cmake_dir.replace('/', '_').upper() + '_FILES'
            f.write('\nset(' + file_tag + '\n')
        
            for file in self.file_lines[cmake_dir]:
                f.write('    ' + file + '\n')
            
            f.write(')\n\n')
            f.write('install(FILES ${%s} DESTINATION ${WB_PACKAGE_SHARED_DIR}/%s)\n' % (file_tag, cmake_dir))
        
        f.writelines(self.file_lines_after)
        f.close()
        

def remove_repository_files(dest):
    entries = os.listdir(dest)

    current_path = os.getcwd()
    os.chdir(dest)

    for entry in entries:
        if entry in files_to_keep:
            continue
                
        if os.path.isfile(entry):
            os.remove(entry)
        else:
            shutil.rmtree(entry)
    
    os.chdir(current_path)
    
    
def recursive_copy(src, dest):
    if os.path.isdir(src):
      
        # Create dir if it doesn't exist in the destination dir
        if not os.path.isdir(dest):
            os.makedirs(dest)
            
        files = os.listdir(src)

        for file in files:
            recursive_copy(os.path.join(src, file), os.path.join(dest, file))
    else:
        shutil.copyfile(src, dest)
        subprocess.Popen(["sed", "-i", "s/\r//g", dest])
        st = os.stat(dest)
        os.chmod(dest, st.st_mode | stat.S_IEXEC)
        
        
def iterate_path(path):
  
  os.chdir(os.path.basename(path))
  current_path = os.getcwd()

  entries = sorted(os.listdir(os.curdir))

  # This would print all the files and directories
  for entry in entries:
    
      full_entry = os.path.join(path, entry)
    
      if entry in banned_files:
        continue
    
      if os.path.isdir(entry):          
          xml_manager.add_directory(full_entry)
          
          iterate_path(full_entry)
          
          os.chdir(current_path)
      else: 
          xml_manager.add_file(path, entry)
          cmake_manager.add_file(full_entry)
          #print entry          
  
  
  
  
  

# TODO: Remove this when done
os.chdir(WORKING_DIR)
print("Preparing...please be patient...")

if clone_repo == True:
    print("  - Checking git proxy configuration...")
    proc = subprocess.Popen(["git", "config", "--global", "http.proxy"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = proc.communicate()

    if not out.strip():
        print("    - WARNING: No proxy configured. Try using git config --global http.proxy <proxy>")
    else:
        print(("    - Using proxy: " + out.strip()))


    print("  - Cloning remote repository...")  

    os.chdir(WORKING_DIR)
    if (os.path.isdir("mysql-sys")):
        shutil.rmtree("mysql-sys")

    proc = subprocess.Popen(["git", "clone", "https://github.com/MarkLeith/mysql-sys"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    out, err = proc.communicate()

    if proc.returncode > 0:
        print(("    - ERROR[" + str(proc.returncode) + "] - There was an error on 'git clone'"))
        print(err)
        sys.exit(1)





print("Processing files...")

xml_manager = XMLFileManager()
cmake_manager = CMakeFileManager()

if clone_repo == True:
    if os.path.isdir('sys') == False:
        os.makedirs('sys')
    remove_repository_files('sys')
    recursive_copy('mysql-sys', 'sys')

if os.path.isdir('sys') == False:
    print('Error: The sys directory does not exist. Please run this script using the --clone parameter.')
    exit(1)

if os.path.isdir('sys/.git'):
    shutil.rmtree(os.path.join('sys', '.git'))

iterate_path('sys')

xml_manager.write_file()
cmake_manager.write_file()


