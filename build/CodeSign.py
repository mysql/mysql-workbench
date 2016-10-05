# Script to sign exe and dll files from WB

import os
#import zipfile
import sys
import shutil

user = sys.argv[1]
passw = sys.argv[2]

if not os.path.exists("MySQLWorkbench.exe"):
    print "This script must be executed from the build output directory (bin\Release)"
    sys.exit(1)    

exclude_files=[
"python.exe",
"python_d.exe", 
"MySQLWorkbench.vshost.exe",
"WBTests.exe",
"Aga.Controls.dll", 
"HTMLRenderer.dll", 
"iconv.dll", 
"intl.dll", 
"libcairo.dll", 
"libglib-2.0-0.dll",
"libgmodule-2.0-0.dll", 
"libgobject-2.0-0.dll", 
"libgthread-2.0-0.dll",
"libpng12-0.dll", 
"libxml2.dll", 
"pcre.dll", 
"python27_d.dll", 
"python27.dll", 
"Scintilla.dll", 
"sqlite3.dll", 
"zlib1.dll"]

files = [f for f in os.listdir(".") if (f.endswith(".exe") or f.endswith(".dll")) and f not in exclude_files]

#print "Creating input zip file"
# Create input zip file
#z = zipfile.ZipFile("../MySQLWorkbench", 'w', zipfile.ZIP_DEFLATED)
#for f in files:
#    print "Zipping", f
#    z.write(f, f)
#z.close()

# Sign everything
print "Sending to codesign server..."
for f in files:
    #os.system("java -Xmx1024m -jar ../../../mysql-gui-win-res/bin/Client.jar -user %s -pass %s -file_to_sign ../MySQLWorkbench -signed_location ../signed -type batchsign -sign_method microsoft" % (user, passw))
    os.system("java -Xmx1024m -jar ../../../mysql-gui-win-res/bin/Client.jar -user %s -pass %s -file_to_sign %s -signed_location ../signed -sign_method microsoft" % (user, passw, f))
    shutil.copy("../signed/%s" % f, f)

print "Unpacking signed files"
# Unpack back everything
#z = zipfile.ZipFile("../signed/MySQLWorkbench", 'r')
#for f in z.namelist():
#    print "Replacing", f
#    o = open(f, 'wb+')
#    o.write(z.read(f))
#    o.close()
#z.close()

#os.remove("../MySQLWorkbench")
#os.remove("../signed/MySQLWorkbench")

print "Done"
