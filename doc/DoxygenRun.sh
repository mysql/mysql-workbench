#doxygen ./doc/Doxyfile
#test -d ./doc/html && cp -f ./doc/tabs.css ./doc/html
cd ..
cat ./doc/Doxyfile | sed -e 's|OUTPUT_DIRECTORY[\t ]*=[\t ]*./doc|OUTPUT_DIRECTORY       = ./doc/ntv/|g' | sed -e 's/GENERATE_TREEVIEW[\t ]*=[\t ]*YES/GENERATE_TREEVIEW      = NO/g' | cat > ./doc/Doxyfile.ntv
doxygen ./doc/Doxyfile.ntv
test -d ./doc/ntv/html && cp -f ./doc/tabs.css ./doc/ntv/html

