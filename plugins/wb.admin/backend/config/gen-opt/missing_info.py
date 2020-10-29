opts = [
{'name': 'ansi', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
#{'name': 'bdb-no-recover', 'values': ({'off':'0', 'on':'1', 'type':'boolean'},)},
#{'name': 'bdb-no-sync', 'values': ({'off':'0', 'on':'1', 'type':'boolean'},)},
#{'name': 'bdb-shared-data', 'values': ({'off':'0', 'on':'1', 'type':'boolean'},)},
{'name': 'bootstrap', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
{'name': 'console', 'values': [{'off':'0', 'on':'1', 'type':'boolean', 'platform':'windows'}]},
{'name': 'des-key-file', 'values': []},
#{'name': 'help', 'values': ()},
{'name': 'innodb_rollback_on_timeout', 'values': []},
{'name': 'log-long-format', 'values': []},
{'name': 'mysql-backup', 'values': []},
{'name': 'safe-mode', 'values': [{'off':'0', 'on':'1', 'type':'boolean'}]},
{'name': 'slave-skip-errors', 'values': []},
{'name': 'symbolic-links', 'values': [{'off':'0', 'on':'1', 'type':'boolean'}], 'disabledby':'skip-symbolic-links'},
{'name': 'verbose', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
#{'name': 'version', 'values': ()},
{'name': 'standalone', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
{'name': 'enable-named-pipe', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
{'name': 'old', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
{'name': 'one-thread', 'values': [{'off':'del', 'on':'name', 'type':'boolean'}]},
#{'name': 'maria', 'values': ()},
#{'name': 'maria-checkpoint-interval', 'values': ()},
#{'name': 'maria-log-dir-path', 'values': ()},
#{'name': 'maria-max-sort-file-size', 'values': ()},
{'name': 'skip-innodb', 'values': [{ 'off': 'del', 'on': 'name', 'type': 'boolean'}]
        , 'caption':'skip-innodb', 'description':'Disable InnoDB', 'versions': ((),((5, 1),), ())
},
{'name': 'concurrent_insert', 'disabledby' : 'skip-concurrent-insert'},
{'name': 'local-infile', 'values': [{ 'off': '0', 'on': '1', 'type': 'boolean', 'default':'true'}]}
]

non_requested = [x['name'] for x in opts]

def get_def(name):
    adef = None
    for o in opts:
        if o['name'] == name:
            adef = o
            non_requested.remove(name)
    return adef
