
from variable_groups import variable_groups


tabs = {}

print(('-----------------------------------\nRunning %s\n-----------------------------------\n' % __file__))


for name, groups in variable_groups:
    for group in groups:
        tab, section = group.split("/", 1)
        if tab not in tabs:
            tabs[tab] = {}
        tabc = tabs[tab]
        if section not in tabc:
            tabc[section] = []
        tabc[section].append(name)

def mycmp(a, b):
    order = ['General', 'Logging', 'InnoDB', 'Networking', 'Security', 'Replication', 'MyISAM', 'Advanced']
    if a[0] in order and b[0] in order:
        return order.index(a[0]) - order.index(b[0])
    return cmp(a[0], b[0])

def mysort(l):
    l.sort(mycmp)
    return l

out = open("options_layout.py", "w+")
out.write("layout = ")
import pprint
pp = pprint.PrettyPrinter(indent=2, stream=out)
pp.pprint(mysort(list([(x,list(y.items())) for x,y in list(tabs.items())])))
out.close()
