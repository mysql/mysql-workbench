import copy
import option
import wb_admin_config_file_be
from wb_common import Users
#===============================================================================
#
#===============================================================================
class DummyPwdHandler:
    def get_password_for(self, name):
        return ""

    def reset_password_for(self, name):
        pass

#===============================================================================
#
#===============================================================================
class HelperDummy:
    content = None
    def __init__(self, content):
        content = self.content

    def get_file_content(file_name, as_user=Users.CURRENT, user_password=None, skip_lines=0):
        return self.content

#===============================================================================
#
#===============================================================================
class CtrlBEDummy:
    def __init__(self):
        self.helper = HelperDummy([])
        self.password_handler = DummyPwdHandler()

#===============================================================================
#
#===============================================================================
class DummyProfile:
    @property
    def target_is_windows(self):
        return False

    @property
    def config_file_section(self):
        return "mysqld"

    @property
    def admin_enabled(self):
        return True

    @property
    def config_file_section(self):
        return "mysqld"


profile = DummyProfile()
#===============================================================================
#
#===============================================================================
def unit_test_1():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start\n", "[mysqld]\n", "ansi\n"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    for name, opt in cfg.options.iteritems():
        values  = opt.get_values()
        default = opt.default_value()
        if len(values) > 0:
            if default is not True:
                if name == 'ansi' and values[0].enabled == True:
                    ret = True

    return (ret, "Reading simple config")

#-------------------------------------------------------------------------------
def unit_test_2():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start\n", "[mysqld]\n", "binlog-do-db=db1\n", "binlog-do-db=db2\n"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    cfg_opts = {}
    for name, opt in cfg.options.iteritems():
        values  = opt.get_values()
        default = opt.default_value()
        if len(values) == 2:
            cfg_opts[id(opt)] = opt

    ret = len(cfg_opts) == 1 and len(cfg_opts.values()[0].get_values()) == 2

    values = cfg_opts.values()[0].get_values()
    ret = ret and values[0].value == "db1" and values[1].value == "db2"

    return (ret, "Reading simple config with multiline option")

#-------------------------------------------------------------------------------
def unit_test_3():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start\n", "[mysqld]\n", "ansi\n"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("chroot")
    values = opt.get_values()
    values.append(option.Value("/var/ww/mydir", True))
    opt.set_values(values)

    new_lines = cfg.generate_file_content()
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(new_lines)

    opt = cfg.get_option("chroot")
    ret = opt.get_values()[0].value == '"/var/ww/mydir"'

    return (ret, "Adding dirname/filename option")

#-------------------------------------------------------------------------------
def unit_test_4():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = []
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False
    new_lines = cfg.generate_file_content()
    ret = '[mysqld]' in new_lines

    return (ret, "Parsing empty config")

#-------------------------------------------------------------------------------
def unit_test_5():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start\n", "[mysqld]\n"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("binlog-do-db")
    values = opt.get_values()
    values.append(option.Value("db1", True))
    values.append(option.Value("db2", True))
    opt.set_values(values)

    new_lines = cfg.generate_file_content()
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(new_lines)

    ret =  'binlog-do-db = db2' in new_lines and 'binlog-do-db = db1' in new_lines

    new_lines = cfg.generate_file_content()
    ret =  ret and 'binlog-do-db = db2' in new_lines and 'binlog-do-db = db1' in new_lines

    return (ret, "Adding multi-line option")

#-------------------------------------------------------------------------------
def unit_test_6():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start", "[mysql]", "port=3306"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("binlog-do-db")
    values = opt.get_values()
    values.append(option.Value("db1", True))
    values.append(option.Value("db2", True))
    opt.set_values(values)

    new_lines = cfg.generate_file_content()
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(new_lines)

    ret =  'binlog-do-db = db2' in new_lines and 'binlog-do-db = db1' in new_lines

    new_lines = cfg.generate_file_content()
    ret =  ret and 'binlog-do-db = db2' in new_lines and 'binlog-do-db = db1' in new_lines

    return (ret, "Adding multi-line option to non-empty file with missing [mysqld]")

#-------------------------------------------------------------------------------
def unit_test_7():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start", "[mysql]", "port=3306", "[mysqld]", 'binlog-do-db = db1', 'binlog-do-db = db2']
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("binlog-do-db")
    values = opt.get_values()
    values[1].value = "db3"

    new_lines = cfg.generate_file_content()
    ret = 'binlog-do-db = db3' in new_lines and 'binlog-do-db = db1' in new_lines

    return (ret, "Changing multi-line option")

#-------------------------------------------------------------------------------
def unit_test_8():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["#Start", "[mysql]", "port=3306", "[mysqld]", 'binlog-do-db = db1', 'binlog-do-db = db2']
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("binlog-do-db")
    values = opt.get_values()
    values[1].enabled = False

    new_lines = cfg.generate_file_content()
    ret = 'binlog-do-db = db2' not in new_lines and 'binlog-do-db = db1' in new_lines

    return (ret, "Deleting one value from a multi-line option")


#-------------------------------------------------------------------------------
def unit_test_9():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["[mysqld]","local-infile = 0",'general-log-file="sdf1"','#general_log_file="sdf2"',"replicate-ignore-db='data1'","replicate-ignore-db='data2'",
"replicate-ignore-db='data3'","port=3306","port=3307"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("replicate-ignore-db")
    values = opt.get_values()
    ret = len(values) == 3

    return (ret, "Testing cfg")

#-------------------------------------------------------------------------------
def unit_test_10():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["[mysqld]","local-infile = 0",'general-log-file="sdf1"','#general_log_file="sdf2"',"replicate-ignore-db='data1'","replicate-ignore-db='data2'",
"replicate-ignore-db='data3'","port=3306","port=3307"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("local-infile")
    (c,r,a) = opt.get_changeset()
    ret = len(c) == len(r) == len(a) == 0

    opt = cfg.get_option("tmpdir")
    values = opt.get_values()
    values.append(option.Value("", True))
    opt.set_values(values)

    #print cfg.generate_file_content()

    return (ret, "Testing cfg")

#-------------------------------------------------------------------------------
def unit_test_11():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["[mysqld]","local-infile = 0",'general-log-file="sdf1"','#general_log_file="sdf2"',"replicate-ignore-db='data1'","replicate-ignore-db='data2'",
"replicate-ignore-db='data3'","port=3306","port=3307"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("skip-networking")
    opt.set_values([option.Value(None, True)])
    opt.set_values([option.Value(None, False)])
    (c, r, a) = opt.get_changeset()

    cont = cfg.generate_file_content()

    return (ret, "Testing cfg")

#-------------------------------------------------------------------------------
def unit_test_12():
    ctrl_be = CtrlBEDummy()
    ctrl_be.helper.content = ["[mysqld]","local-infile = 0",'general-log-file="sdf1"','#general_log_file="sdf2"',"replicate-ignore-db='data1'","replicate-ignore-db='data2'",
"replicate-ignore-db='data3'","port=3306","port=3307","skip-networking"]
    cfg = wb_admin_config_file_be.ConfigFileSource((5,1,8), ctrl_be, profile)
    cfg.file_name = "dummy_name"
    cfg.parse_file_contents(ctrl_be.helper.content)

    ret = False

    opt = cfg.get_option("skip-networking")
    values = opt.get_values()
    opt.set_values([option.Value(None, False)])
    (c, r, a) = opt.get_changeset()

    cont = cfg.generate_file_content()
    print cont

    return (ret, "Testing cfg")











#-------------------------------------------------------------------------------
results = []
tests = sorted(filter(lambda x: "unit_test" in x, dir()))

tests_numbers = sorted(map(lambda x: int(x[10:]), tests))

for testnr in tests_numbers:
    test = "unit_test_%i" % testnr
    print "--------------------------------------------------------------------"
    print "-- %s starting" % test
    ret = eval(test + "()")
    print "-- %s %s:   %s\n" % (test, str(ret[0]), ret[1])
    results.append((ret[0], ret[1], test))



print "============================== SUMMARY ==================================="
for res in results:
    ch = "+" if res[0] else "-"
    print "%s %s - %s" % (ch, res[2], res[1])
