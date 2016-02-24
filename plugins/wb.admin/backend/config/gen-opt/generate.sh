rm -f *.pyc
python -B 1_mysqld2optlist.py
python -B 2_genoptions_layout.py
python -B 3_rawopts2opts.py

if [ "$1" == "update" ]; then
    echo "Copying files to the code directory..."
    cp opts.py ../..
    cp wb_admin_variable_list.py ../..
fi
