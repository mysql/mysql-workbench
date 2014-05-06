rm -f *.pyc
python -B 1_mysqld2optlist.py
python -B 2_genoptions_layout.py
python -B 3_rawopts2opts.py
