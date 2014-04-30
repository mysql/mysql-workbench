#import wb_admin_utils
import os
import sys
import glob

import unittest

import grt
import wb_admin_control
import wb_server_control

try:
    import coverage
except ImportError:
    has_coverage = False
else:
    has_coverage = True

CURRENT_DIR = os.path.abspath(os.path.dirname(__file__))
SRC_DIR = os.path.abspath(os.path.join(CURRENT_DIR, '../../../'))

sys.path[:0] = [ os.path.join(SRC_DIR, 'testing/python') ]
import db_test_fixtures

test_params_local={'hostName':'127.0.0.1', 'port':3306, 'user':'testuser', 'password':'c3poLATxce090'
                   , 'login':{'ssh.hostName':'', 'ssh.key':'', 'ssh.localPort':'', 'ssh.password':'', 'ssh.useKey':'', 'ssh.userName':''}
                   , 'server':{'remoteAdmin'        : 0,
                               'serverVersion'      : '5.5.15',
                               'sys.config.path'    : os.path.join(CURRENT_DIR, 'fixtures/my.cnf'),
                               'sys.config.section' : 'mysqld',
                               'sys.mysqld.start'   : '/etc/init.d/mysql start',
                               'sys.mysqld.status'  : 'ps -C mysqld -o pid=',
                               'sys.mysqld.stop'    : '/etc/init.d/mysql stop',
                               'sys.preset'         : 'Ubuntu Linux (Vendor Package)',
                               'sys.sudo'           : '/usr/bin/sudo -p EnterPasswordHere /bin/sh -c',
                               'sys.system'         : 'Linux',
                               'sys.usesudo'        : 1,
                               'sys.usesudostatus'  : 0,
                               'sys.script'         :  'get_cpu_info = /usr/bin/uptime | wba_token(' ', -3)'
                                                      +'get_mem_info = free | wba_filter(Mem:) | wba_token(\' \', 3)'
                                                      +'get_mem_total = free | wba_filter(Mem:) | wba_token(\' \', 1)'
                              } # end of server
                  }

test_params_remote={'hostName':'127.0.0.1', 'port':3306, 'user':'testuser', 'password':'c3poLATxce090'}


def create_instance(hostName, port, user, password, login, server):
    driver = None
    for i in grt.root.wb.rdbmsMgmt.rdbms:
        if i.name == 'Mysql':
            driver = i.defaultDriver
            break

    conn = None
    if driver:
        conn = grt.classes.db_mgmt_Connection()
        conn.driver = driver

    conn.name = hostName + ":" + str(port)
    conn.hostIdentifier = "Mysql@" + conn.name
    params = conn.parameterValues
    params['hostName'] = hostName
    params['port'] = port
    params['userName'] = user
    params['%userName%::Mysql@%hostName%:%port%'] = '%userName%::Mysql@%hostName%:%port%'
    params['password'] = password

    instance = grt.classes.db_mgmt_ServerInstance()
    instance.connection = conn

    # Create login part
    li = instance.loginInfo
    for k,v in login.iteritems():
        li[k] = v

    # Create server part
    si = instance.serverInfo
    for k,v in server.iteritems():
        si[k] = v

    return instance

class AdminTestCase(unittest.TestCase):
    local_server_profile = wb_server_control.ServerProfile(create_instance(**test_params_local))
    local_ctrl_be = wb_admin_control.WbAdminControl(local_server_profile)
    local_ctrl_be.init()
    local_db_fixtures = db_test_fixtures.DBFixtures(test_params_local)

if __name__ == '__main__':
    if has_coverage:
        cov = coverage.coverage()
        cov.start()

    os.chdir(CURRENT_DIR)
    sys.path[:0] = [ CURRENT_DIR ]

    suite = unittest.TestSuite()
    names = [ os.path.splitext(fname)[0] for fname in glob.glob("test_*.py") ]
    suite.addTest(unittest.defaultTestLoader.loadTestsFromNames(names))

    result = unittest.TextTestRunner(stream=open(os.path.join(SRC_DIR, 'testing/python/test_results_wb.admin.txt'), 'w'), verbosity=2).run(suite)

    if has_coverage:
        cov.stop()
        report_file = open(os.path.join(SRC_DIR, 'testing/python/test_results_coverage_report.txt'), 'a')
        cov.report(file=report_file)
        report_file.write('\n\n')

    if not result.wasSuccessful():
        sys.exit(1)


