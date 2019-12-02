#!/usr/bin/python
import xml.dom.minidom
import re
import pprint
import missing_info
from functools import reduce

skip_list = set(['version', 'help', 'license', 'version_comment', 'version_compile_machine', 'version_compile_os'])

# variables that should be also treated as mycnf options
mycnf_vars = []

# table_type is a synonym to storage_engine
OPTIONS_TO_IGNORE = ["default-table-type", "new"]

doc = xml.dom.minidom.parse('mysqld.xml')

opts = []
sys_vars = []
stat_vars = []
stat_var_sections = []

bool_tuples = (('off', 'on'), ('no','yes'), ('0','1'), ('false','true'))
bool_dict = {}
for tup in bool_tuples:
    bool_dict[tup[0]] = tup
    bool_dict[tup[1]] = tup

tags = {}
types = {}

print(('-----------------------------------\nRunning %s\n-----------------------------------\n' % __file__))


def count_tag(dic, tag):
    if tag in dic:
        dic[tag] += 1
    else:
        dic[tag] = 1

#-------------------------------------------------------------------------------
def parse_boolean(node, value, opt):
    disabledby = None
    if 'disabledby' in opt:
        disabledby = opt['disabledby']
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE:
            if t.tagName == 'value':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'default':
                        if aname in value:
                            print("existing bool value tag", aname, value)
                        else:
                            value[str(aname)] = str(aval.lower())
                    else:
                        print("Uknown attr in value", aname, opt['name'])
                        quit()
            elif t.tagName == 'choice':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'value':
                        if 'choice' in value:
                            value['choice'].append(str(aval.lower()))
                        else:
                            value['choice'] = [str(aval.lower())]
                    else:
                        print("Uknown attr in choice", aname, opt['name'])
                        quit()
            else:
                print("Warning unkown tag in boolean value", aname, aval)
                quit()

    default = None
    if 'default' in value:
        default = value['default'].lower()

    switch = bool_dict['0'] # pick 0|1 as a default assignable items

    if default in bool_dict:
        switch = bool_dict[default]

    if disabledby is None:
        value['off'] = switch[0]
    else:
        value['off'] = 'disabledby'

    value['on'] = switch[1]

#-------------------------------------------------------------------------------
def parse_set(node, value, opt):
    parse_enumeration(node, value, opt)
    value['type'] = 'set'

#-------------------------------------------------------------------------------
def parse_string(node, value, opt):
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE:
            if t.tagName == 'value':
                for (aname, aval) in list(t.attributes.items()):
                    if aname in value:
                        print("existing str value tag", aname, value)
                    else:
                        value[str(aname)] = str(aval)
                        if aname == 'default' and str(aval) == "empty string":
                            value[str(aname)] = ""
            elif t.tagName == 'choice':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'value':
                        if 'choice' in value:
                            value['choice'].append(str(aval))
                        else:
                            value['choice'] = [str(aval)]
                    else:
                        print("Uknown attr in choice", aname, opt['name'])
                        quit()
            else:
                print("Warning unkown tag in boolean value", aname, aval)
                quit()

#-------------------------------------------------------------------------------
def parse_numeric(node, value, opt):
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE:
            if t.tagName == 'value':
                for (aname, aval) in list(t.attributes.items()):
                    if aname in value:
                        print("existing num value tag", opt.get("name"), aname, value)
                    else:
                        value[str(aname)] = str(aval)
            elif t.tagName == 'choice':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'value':
                        if 'choice' in value:
                            value['choice'].append(str(aval))
                        else:
                            value['choice'] = [str(aval)]
                    else:
                        print("Uknown attr in choice", aname, opt['name'])
                        quit()
            else:
                print("Warning unkown tag in boolean value", aname, aval)
                quit()

#-------------------------------------------------------------------------------
def parse_filename(node, value, opt):
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE:
            if t.tagName == 'value':
                for (aname, aval) in list(t.attributes.items()):
                    value[str(aname)] = str(aval)
                    if aname == 'default' and str(aval) == "empty string":
                        value[str(aname)] = ""

#-------------------------------------------------------------------------------
def parse_enumeration(node, value, opt):
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE:
            if t.tagName == 'value':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'default':
                        if aname in value:
                            print("existing enum value tag", aname, value)
                        else:
                            value[str(aname)] = str(aval)
                    elif aname == 'se':
                        pass
                    else:
                        print("Uknown attr in value", aname, opt['name'])
#quit()
            elif t.tagName == 'choice':
                for (aname, aval) in list(t.attributes.items()):
                    if aname == 'value':
                        if 'choice' in value:
                            value['choice'].append(str(aval))
                        else:
                            value['choice'] = [str(aval)]
                    else:
                        print("Uknown attr in choice", aname, opt['name'])
                        quit()
            else:
                print("Warning unkown tag in boolean value", aname, aval)
                quit()
    value['type'] = 'enum'

#-------------------------------------------------------------------------------
def parse_bitmap(node, value, opt):
    pass

#-------------------------------------------------------------------------------
def parse_date_time(node, value, opt):
    pass

#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
type_parsers = {'boolean':parse_boolean
               ,'set':parse_set
               ,'string':parse_string
               ,'numeric':parse_numeric
               ,'filename':parse_filename
               ,'dirname':parse_filename
               ,'enumeration':parse_enumeration
               ,'bitmap':parse_bitmap
               ,'datetime':parse_date_time
               ,'integer':parse_numeric
               }

#-------------------------------------------------------------------------------
def parse_version_str(version_str):
    version = None
    try:
        res = re.match("([0-9]+)\.([0-9]+)\.([0-9]+)|([0-9]+)\.([0-9]+)", version_str)
        if res:
            tokens = res.groups()
            if tokens[0] is not None:
                version = (int(tokens[0]), int(tokens[1]), int(tokens[2]))
            else:
                version = (int(tokens[3]), int(tokens[4]))
    except ValueError:
        print("ERROR! incorrect version attribute value '" + version_str + "', ", type(version_str))

    return version

#-------------------------------------------------------------------------------
def parse_values(node, opt):
    value = {}
    for n,v in list(node.attributes.items()):
        value[str(n)] = str(v)

    vartype = str(value['vartype'])

    value['type'] = vartype
    del value['vartype']

    type_parsers[vartype](node, value, opt)

    if 'platform' in value:
        if value['platform'] == 'all':
            del value['platform']

    if 'inversion' in value:
        value['inversion'] = parse_version_str(value['inversion'])
    if 'outversion' in value:
        value['outversion'] = parse_version_str(value['outversion'])

    return value

#-------------------------------------------------------------------------------
def guess_values(opt):
    name = opt['name']
    values = opt['values']
    if len(values) == 0:
        if name[:4] == 'skip':
            values.append({'off':'del', 'on':'name', 'type':'boolean'})

#-------------------------------------------------------------------------------
def parse_optype(node, option):
    optype = {}
    for i in list(node.attributes.items()):
        optype[str(i[0])] = str(i[1])

    # make cleanup
    if optype:
        if 'format' in optype:
            if optype['format'] == option['name']:
                del optype['format']
    return optype

#-------------------------------------------------------------------------------
def ver_cmp(v1, v2):
    minlen = min(len(v1), len(v2))
    return cmp(v1[0:minlen], v2[0:minlen])

#-------------------------------------------------------------------------------
def ver_diff(v1,v2):
    return abs(reduce(lambda x,y: x*10 + y, v1) - reduce(lambda x,y: x*10 + y, v2))

#-------------------------------------------------------------------------------
def parse_versions(versions_node, option):
    vers = []
    removed = []
    introduced = []
    for node in versions_node.childNodes:
        if xml.dom.Node.ELEMENT_NODE == node.nodeType:
            if node.tagName == "manual":
                version_str = node.getAttribute('version')
                version = parse_version_str(version_str)
                if version:
                    if len(version) > 2:
                        print("Error: manual version is set in x.y.z format", option)
                        quit()
                    vers.append(version)
            elif node.tagName == 'introduced':
                version_str = node.getAttribute('version')
                version = parse_version_str(version_str)
                if version:
                    introduced.append(version)
            elif node.tagName == "removed":
                version_str = node.getAttribute('version')
                version = parse_version_str(version_str)
                if version:
                    removed.append(version)

    vers.sort()
    vers.append((0,0))
    res = []
    skip = False
    for i in range(0, len(vers)-1):
        if vers[i][0:2] == vers[i+1][0:2]:
            res.append(max(vers[i], vers[i+1]))
            skip = True
        else:
            if not skip:
                res.append(vers[i])
            else:
                skip = False

    return [tuple(introduced), tuple(res), tuple(removed)]

#-------------------------------------------------------------------------------
def parse_types(node, option):
    optype = None
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE and t.tagName == 'mycnf':
            cur_optype = parse_optype(t, option)
            if optype is None and cur_optype is not None:
                optype = cur_optype
    return optype

#-------------------------------------------------------------------------------
def parse_var_types(node, option):
    for t in node.childNodes:
        if t.nodeType == t.ELEMENT_NODE and t.hasAttribute('isdynamic'):
            if t.getAttribute('isdynamic') not in ('yes','no'):
                print("Invalid isdynamic value", t.getAttribute('isdynamic'))
            return t.getAttribute('isdynamic') == 'yes', t.nodeName
    return None, None

#-------------------------------------------------------------------------------

def check_redundant_option(opts, optid):
    if '-' in optid and optid.replace('-', '_') in [o['name'] for o in opts]:
        print('Skipped redundant option: %s'%o['name'])
        return True
    elif '_' in optid and optid.replace('_', '-') in [o['name'] for o in opts]:
        for o in opts:
            if optid.replace('_', '-') == o['name']:
                opts.remove(o)
                print('Removed redundant option: %s'%o['name'])
    return False
#-------------------------------------------------------------------------------
for option in doc.documentElement.getElementsByTagName('mysqloption'):
    opt = {}
    optid = str(option.getAttribute('id'))
    opt['name'] = optid.strip()
    opt['caption'] = optid.strip()
    is_mycnf_opt = False
    is_variable_dynamic = False
    variable_class = None
    opt['values'] = []

    for t in option.getElementsByTagName('types'):
        otype = parse_types(t, opt)
        if otype is not None:
            is_mycnf_opt = True
            if len(otype) > 0:
                opt['optype'] = otype
        is_variable_dynamic, variable_class = parse_var_types(t, opt)

    for t in option.getElementsByTagName('disabledby'):
        items = list(t.attributes.items())
        # items usually look like [('xref', 'skip-merge')] for optid 'merge'
        if len(items) == 1:
            items = items[0]
            opt['disabledby'] = str(items[1])
        else:
            print("Warning: disabledby has more that one value or no values", optid, items)

    for node in option.childNodes:
        if node.nodeType == node.ELEMENT_NODE:
            if node.tagName == 'values':
                ovalue = parse_values(node, opt)
                opt['values'].append(ovalue)
            elif node.tagName == 'versions':
                opt['versions'] = parse_versions(node, opt)
            elif node.tagName == 'description':
                firstChild = node.firstChild
                desc = firstChild.data.encode('ascii').replace("\n", "") if firstChild else ""
                d1 = desc
                d2 = desc.replace('  ', ' ')
                while d1 != d2:
                    d1 = d2
                    d2 = d2.replace('  ', ' ')
                desc = d2
                opt['description'] = desc.strip(" ")
                #if len(desc) > 80:
                #  f = open('long_desc.txt','a')
                #  f.write("option '" + optid + "' has long desc. " + desc + "\n")
                #  f.close()
            elif node.tagName == 'deprecated':
                version = node.getAttribute('version')
                opt['deprecated'] = parse_version_str(version)

    if is_mycnf_opt or optid in mycnf_vars:#or variable_class == "system":
        if optid[:5] != 'maria' and optid[:6] != 'falcon' and optid[:3] != 'bdb' and (not optid in skip_list):
            if not check_redundant_option(opts, optid):
                opts.append(opt)

        if 'values' in opt and len(opt['values']) == 0:
            guess_values(opt)
    elif variable_class == "system":
        print("-", optid)

    if is_variable_dynamic is not None:
        if variable_class == "system":
            var = {'name':opt['name'], 'description':opt.get('description', ''), 'dynamic':is_variable_dynamic}
            sys_vars.append(var)
        else:
            var = {'name':opt['name'], 'description':opt.get('description', ''), 'dynamic':is_variable_dynamic}
            stat_vars.append(var)
            stat_var_sections.append((opt['name'], option.getAttribute('section').encode('latin1')))


#-------------------------------------------------------------------------------
# Merge from missing_info
for o in opts:
    missing_def = missing_info.get_def(o['name'])
    if missing_def:
        for k,v in missing_def.items():
            o[k] = v
        print("Merged", missing_def)
        print("    to", o)
        print("----")

for o in missing_info.non_requested:
    missing_def = missing_info.get_def(o)
    opts.append(missing_def)

#-------------------------------------------------------------------------------
# make list const
for o in opts:
    if 'values' in o and len(o['values']) > 0:
        o['values'] = values = list(o['values'])
for o in opts:
    if not 'values' in o or len(o['values']) == 0:
        print("Warning: option", o['name'], "has no values")

    if 'skip-' in o['name']:
        for opt in opts:
            if o['name'][5:] == opt['name']:
                for v in opt['values']:
                    if 'default' in v:
                        if v['default'].lower() in ['1', 'on', 'true', 'yes']:
                            print('Removed option %s'%(opt['name']))
                            opts.remove(opt)
                        else:
                            print('Removed option %s'%(o['name'])) 
                            opts.remove(o)
                        break
                break

print("Writing to raw_opts.py...")
f = open('raw_opts.py', 'w+')
f.write("ropts = ")
pp = pprint.PrettyPrinter(indent=2, stream=f)
pp.pprint(opts)
f.close()

print("Writing to wb_admin_variable_list.py...")
f = open('raw_vars.py', 'w+')
f.write("system_vars_list =")
pp = pprint.PrettyPrinter(indent=2, stream=f)
pp.pprint(sys_vars)
f.write("\n\n")
f.write("status_vars_list =")
pp.pprint(stat_vars)
f.close()

#f = open("variable_groups.py","w+")
#pp = pprint.PrettyPrinter(indent=2, stream=f)
#pp.pprint(stat_var_sections)
#f.close()
