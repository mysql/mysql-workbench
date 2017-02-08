# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

from workbench.db_utils import QueryError, escape_sql_string
from workbench.utils import Version
from wb_common import PermissionDeniedError

import mforms
from workbench.log import log_error


class WBSecurityValidationError(Exception):
    pass


LIST_ACCOUNTS_QUERY = "SELECT User, Host FROM mysql.user ORDER BY User"
LIST_SCHEMAS_QUERY = "SHOW DATABASES"

ZOMBIE_SCHEMA_PRIVS_QUERY = "SELECT d.User, d.Host, d.Db FROM mysql.db AS d LEFT JOIN mysql.user AS u ON d.User = u.user AND d.Host = u.Host WHERE u.User IS NULL"
ZOMBIE_TABLE_PRIVS_QUERY = "SELECT t.User, t.Host, t.Db, t.Table_name FROM mysql.tables_priv AS t LEFT JOIN mysql.user AS u ON t.User = u.user AND t.Host = u.Host WHERE u.User IS NULL"
ZOMBIE_COLUMN_PRIVS_QUERY = "SELECT c.User, c.Host, c.Db, c.Table_name, c.Column_name FROM mysql.columns_priv AS c LEFT JOIN mysql.user AS u ON c.User = u.user AND c.Host = u.Host WHERE u.User IS NULL"
ZOMBIE_PROCS_PRIVS_QUERY = "SELECT p.User, p.Host, p.Db, p.Routine_name, p.Routine_type FROM mysql.procs_priv AS p LEFT JOIN mysql.user AS u ON p.User = u.user AND p.Host = u.Host WHERE u.User IS NULL"


GET_ACCOUNT_QUERY = "SELECT * FROM mysql.user WHERE User='%(user)s' AND Host='%(host)s' ORDER BY User, Host"

GET_ACCOUNT_SCHEMA_PRIVS_QUERY = "SELECT * FROM mysql.db WHERE User='%(user)s' AND Host='%(host)s' ORDER BY Db"

GET_ACCOUNT_MYSQL_TABLE_PRIVS_QUERY = "SELECT * FROM mysql.tables_priv WHERE Host='%(host)s' AND User='%(user)s' AND Db='mysql'"
#GET_ACCOUNT_IS_TABLE_PRIVS_QUERY = "SELECT * FROM mysql.tables_priv WHERE Host='%(host)s' AND User='%(user)s' AND Db='information_schema'"

CREATE_USER_QUERY = "CREATE USER '%(user)s'@'%(host)s' IDENTIFIED BY '%(password)s'"
CREATE_USER_QUERY_PLUGIN_AUTH_STRING = "CREATE USER '%(user)s' IDENTIFIED WITH '%(auth_plugin)s' AS '%(auth_string)s'"
CREATE_USER_QUERY_PLUGIN = "CREATE USER '%(user)s' IDENTIFIED WITH '%(auth_plugin)s'"

GRANT_GLOBAL_PRIVILEGES_QUERY = "GRANT %(granted_privs)s ON *.* TO '%(user)s'@'%(host)s'"  # A WITH clause will be added if needed
REVOKE_GLOBAL_PRIVILEGES_QUERY = "REVOKE %(revoked_privs)s ON *.* FROM '%(user)s'@'%(host)s'"
GRANT_LIMITS_QUERY = "GRANT USAGE ON *.* TO '%(user)s'@'%(host)s' WITH %(limit)s"
RENAME_USER_QUERY = "RENAME USER '%(old_user)s'@'%(old_host)s' TO '%(user)s'@'%(host)s'"
CHANGE_PASSWORD_QUERY = "SET PASSWORD FOR '%(user)s'@'%(host)s' = PASSWORD('%(password)s')"
BLANK_PASSWORD_QUERY = "SET PASSWORD FOR '%(user)s'@'%(host)s' = ''"
CHANGE_PASSWORD_QUERY_576 = "ALTER USER '%(user)s'@'%(host)s' IDENTIFIED BY '%(password)s'"
BLANK_PASSWORD_QUERY_576 = "ALTER USER '%(user)s'@'%(host)s' IDENTIFIED BY ''"

REVOKE_SCHEMA_PRIVILEGES_QUERY = "REVOKE %(revoked_privs)s ON `%(db)s`.* FROM '%(user)s'@'%(host)s'"
GRANT_SCHEMA_PRIVILEGES_QUERY = "GRANT %(granted_privs)s ON `%(db)s`.* TO '%(user)s'@'%(host)s'"

EXPIRE_PASSWORD = "ALTER USER '%(user)s'@'%(host)s' PASSWORD EXPIRE"
FLUSH_PRIVILEGES = "FLUSH PRIVILEGES"

REVOKE_ALL = "REVOKE ALL PRIVILEGES, GRANT OPTION FROM '%(user)s'@'%(host)s'"
REMOVE_USER = "DROP USER '%(user)s'@'%(host)s'"

# Map of user table's column name to privilege and its description.
# It has the form { 'TableColumnPrivName': ('PrettyName', 'ADescriptionOfThePrivilege'), ... }
PrivilegeInfo = {
"Select_priv": ("SELECT", "The SELECT privilege enables you to select rows from tables in a database.\nSELECT statements require the SELECT privilege only if they actually retrieve rows from a table. Some SELECT statements do not access tables and can be executed without permission for any database"),
"Insert_priv": ("INSERT", "The INSERT privilege enables you to be inserted into tables in a database.\nINSERT is also required for the ANALYZE TABLE, OPTIMIZE TABLE, and REPAIR TABLE table-maintenance statements."),
"Update_priv": ("UPDATE", "The UPDATE privilege enables you to be updated in tables in a database."),
"Delete_priv": ("DELETE", "The DELETE privilege enables you to be deleted from tables in a database."),
"Create_priv": ("CREATE", "The CREATE privilege enables creation of new databases and tables."),
"Drop_priv": ("DROP", """The DROP privilege enables you to drop (remove) existing databases, tables, and views.
Beginning with MySQL 5.1.10, the DROP privilege is also required in order to use the statement ALTER TABLE ... DROP PARTITION on a partitioned table.
Beginning with MySQL 5.1.16, the DROP privilege is required for TRUNCATE TABLE (before that, TRUNCATE TABLE requires the DELETE privilege).
If you grant the DROP privilege for the mysql database to a user, that user can drop the database in which the MySQL access privileges are stored."""),
"Reload_priv": ("RELOAD", "The RELOAD privilege enables use of the FLUSH statement.\nIt also enables mysqladmin commands that are equivalent to FLUSH operations:\nflush-hosts, flush-logs, flush-privileges, flush-status, flush-tables, flush-threads, refresh, and reload."),
"Shutdown_priv": ("SHUTDOWN", "The SHUTDOWN privilege enables use of the mysqladmin shutdown command. There is no corresponding SQL statement."),
"Process_priv": ("PROCESS", "The PROCESS privilege pertains to display of information about the threads executing within the server\n(that is, information about the statements being executed by sessions).\nThe privilege enables use of SHOW PROCESSLIST or mysqladmin processlist to see threads belonging to other accounts; you can always see your own threads."),
"File_priv": ("FILE", """The FILE privilege gives you permission to read and write files on the server host using the
LOAD DATA INFILE and SELECT ... INTO OUTFILE statements and the LOAD_FILE() function. A user who has the FILE privilege
can read any file on the server host that is either world-readable or readable by the MySQL server.
(This implies the user can read any file in any database directory, because the server can access any of those files.)
The FILE privilege also enables the user to create new files in any directory where the MySQL server has write access.
As a security measure, the server will not overwrite existing files."""),
"Grant_priv": ("GRANT OPTION", "The GRANT OPTION privilege enables you to give to other users or remove from other users those privileges that you yourself possess."),
"References_priv": ("REFERENCES", "The REFERENCES privilege currently is unused."),
"Index_priv": ("INDEX", "The INDEX privilege enables you to create or drop (remove) indexes.\nINDEX applies to existing tables. If you have the CREATE privilege for a table, you can include index definitions in the CREATE TABLE statement."),
"Alter_priv": ("ALTER", "The ALTER privilege enables use of ALTER TABLE to change the structure of or rename tables.\n(ALTER TABLE also requires the INSERT and CREATE privileges.)"),
"Show_db_priv": ("SHOW DATABASES", """The SHOW DATABASES privilege enables the account to see database names by issuing the
SHOW DATABASE statement. Accounts that do not have this privilege see only databases for which
they have some privileges, and cannot use the statement at all if the server was started with
the --skip-show-database option. Note that any global privilege is a privilege for the database."""),
"Super_priv": ("SUPER", """The SUPER privilege enables an account to use CHANGE MASTER TO, KILL or
mysqladmin kill to kill threads belonging to other accounts (you can always kill your own threads),
PURGE BINARY LOGS, configuration changes via SET GLOBAL to modify global system variables,
the mysqladmin debug command, enabling or disabling logging, performing updates even if the read_only
system variable is enabled, starting and stopping replication on slave servers, and allows you to
connect (once) even if the connection limit controlled by the max_connections system variable is reached."""),

"Create_tmp_table_priv": ("CREATE TEMPORARY TABLES", "The CREATE TEMPORARY TABLES privilege enables the use of the keyword TEMPORARY in CREATE TABLE statements."),
"Lock_tables_priv": ("LOCK TABLES", "The LOCK TABLES privilege enables the use of explicit LOCK TABLES statements to lock tables for which you have the SELECT privilege.\nThis includes the use of write locks, which prevents other sessions from reading the locked table."),
"Execute_priv": ("EXECUTE", "The EXECUTE privilege is required to execute stored routines (procedures and functions)."),
"Repl_slave_priv": ("REPLICATION SLAVE", "The REPLICATION SLAVE privilege should be granted to accounts that are used\nby slave servers to connect to the current server as their master.\nWithout this privilege, the slave cannot request updates that have been made to databases on the master server."),
"Repl_client_priv": ("REPLICATION CLIENT", "The REPLICATION CLIENT privilege enables the use of SHOW MASTER STATUS and SHOW SLAVE STATUS"),
"Create_view_priv": ("CREATE VIEW", "The CREATE VIEW privilege enables use of CREATE VIEW."),
"Show_view_priv": ("SHOW VIEW", "The SHOW VIEW privilege enables use of SHOW CREATE VIEW."),
"Create_routine_priv": ("CREATE ROUTINE", "The CREATE ROUTINE privilege is needed to create stored routines (procedures and functions)."),
"Alter_routine_priv": ("ALTER ROUTINE", "The ALTER ROUTINE privilege is needed to alter or drop stored routines (procedures and functions)."),
"Create_user_priv": ("CREATE USER", "The CREATE USER privilege enables use of CREATE USER, DROP USER, RENAME USER, and REVOKE ALL PRIVILEGES."),
"Event_priv": ("EVENT", "The EVENT privilege is required to create, alter, or drop events for the Event Scheduler. This privilege was added in MySQL 5.1.6."),
"Trigger_priv": ("TRIGGER", "The TRIGGER privilege enables you to create and drop triggers.\nYou must have this privilege for a table to create or drop triggers for that table. This privilege was added in MySQL 5.1.6. (Prior to MySQL 5.1.6, trigger operations required the SUPER privilege.)"),
"Create_tablespace_priv": ("CREATE TABLESPACE", "The CREATE TABLESPACE privilege is needed to create, alter, or drop tablespaces and log file groups."),

# custom stuff
#"IS_monitor_attr" : ("* INFO SCHEMA monitoring", ""),
"Routine_manage_attr" : ("* Modify Routines", ""),
#"IS_read_all_attr" : ("* SELECT access to INFO SCHEMA", ""),
"User_manage_attr" : ("* Modify Access Control", ""),
#"IS_monitor_innodb_attr" : ("* InnoDB monitoring in INFO SCHEMA", "")
}

PrivilegeReverseDict = dict( (val[0], key) for key, val in PrivilegeInfo.iteritems() )

## Special Tables

# information_schema tables are free for all to see

AdminAttributes = {
#             Db, Tables, Db Privileges
#"IS_monitor_attr" : ("information_schema",
#              ["statistics", "engines", "plugins", "partitions", "files", "processlist", "global_status",
#              "session_status", "global_variables", "session_variables", "profiling"],
#              ["Select_priv"],  None, None),
"Routine_manage_attr" : ("mysql",
              ["proc", "func", "event"],
              ["Insert", "Select", "Update", "Delete"],
              "GRANT INSERT, SELECT, UPDATE, DELETE ON TABLE mysql.`%(table)s` TO '%(user)s'@'%(host)s'",
              "REVOKE INSERT, SELECT, UPDATE, DELETE ON TABLE mysql.`%(table)s` FROM '%(user)s'@'%(host)s'"),
#"IS_read_all_attr" : ("information_schema",
#              ["*"],
#              ["Select_priv"]),
"User_manage_attr" : ("mysql",
              ["columns_priv", "db", "host", "procs_priv", "tables_priv", "user"],
              ["Select", "Insert", "Update", "Delete"],
              "GRANT INSERT, SELECT, UPDATE, DELETE ON TABLE mysql.`%(table)s` TO '%(user)s'@'%(host)s'",
              "REVOKE INSERT, SELECT, UPDATE, DELETE ON TABLE mysql.`%(table)s` FROM '%(user)s'@'%(host)s'"),
#"IS_monitor_innodb_attr" : ("information_schema",
#              [],  # InnoDB_* (5.4+ only)
#              ["Select_priv"]),
}


# Administrative Roles available for selection. Add your custom roles here.
# Format is ("NameOfTheRole", "description", [list,of,privileges])
# For a list of available privileges, look at the mysql.user table
SecurityAdminRoles = [
("DBA", "grants the rights to perform all tasks",
    PrivilegeInfo.keys()),
("MaintenanceAdmin", "grants rights needed to maintain server",
    ["Event_priv", "Reload_priv", "Show_db_priv", "Shutdown_priv", "Super_priv"]), # , "IS_monitor_attr" File_priv?
("ProcessAdmin", "rights needed to assess, monitor, and kill any user process running in server",
    ["Reload_priv", "Super_priv"]),#, "IS_monitor_attr"
("UserAdmin", "grants rights to create users logins and reset passwords",
    ["Create_user_priv", "Reload_priv"]),
("SecurityAdmin", "rights to manage logins and grant and revoke server and database level permission",
    ["Grant_priv", "Create_user_priv", "Reload_priv", "Show_db_priv", "User_manage_attr"]),
("MonitorAdmin", "minimum set of rights needed to monitor server",
    ["Process_priv"]), #"IS_monitor_attr", "IS_monitor_innodb_attr"
("DBManager", "grants full rights on all databases",
    ["Create_priv", "Drop_priv", "Grant_priv", "Event_priv", "Alter_priv", "Delete_priv",
    "Index_priv", "Insert_priv", "Select_priv", "Update_priv", "Create_tmp_table_priv", "Lock_tables_priv",
    "Trigger_priv", "Create_view_priv", "Show_view_priv", "Create_routine_priv", "Alter_routine_priv", "Show_db_priv"]),
("DBDesigner", "rights to create and reverse engineer any database schema",
    ["Create_priv", "Alter_priv", "Index_priv", "Trigger_priv", "Create_view_priv",
    "Show_view_priv", "Create_routine_priv", "Alter_routine_priv", "Show_db_priv", "Routine_manage_attr"]),#, "IS_read_all_attr"
("ReplicationAdmin", "rights needed to setup and manage replication",
    ["Repl_client_priv", "Repl_slave_priv", "Super_priv"]),
("BackupAdmin", "minimal rights needed to backup any database",
    ["Event_priv", "Select_priv", "Lock_tables_priv", "Show_db_priv"])
]



# for sorting tuples containing host, db, user entries in the same order as the server
def get_acl_sort(tup):
    if type(tup) is str:
        tup = tup,
    order = 0

    wild_pos = 0
    chars = 0
    for t in tup:
        if type(t) is not str:
            continue
        for i in range(len(t)):
            c = t[i]
            if c == "\\":
                pass
            elif c == "%" or c == "_":
                wild_pos = i
                break
            chars = 128
        order = (order << 8) + (wild_pos and min(wild_pos, 127) or chars)
    return order


def acl_compare(t1, t2):
    return get_acl_sort(t1) - get_acl_sort(t2)


def escape_schema_name(s):
    return s.replace("\\", "\\\\").replace("_", "\_").replace("%", "\%")


class AdminSecurity(object):
    def __init__(self, ctrl_be):
        self.ctrl_be = ctrl_be
        self._accounts = []
        self._schema_names = []
        self._zombie_privs = {} # (user, host): [objects]

        self._account_info_cache = {}  # A mapping with the form 'user@host' -> AdminAccount instance for that user
        self._schema_privileges_cache = {}

        self.has_plugin = False
        self.has_authentication_string = False
        self.has_max_user_connections = False
        self.has_password_expired = False

        # Supported schema specific privilege list:
        self.schema_privilege_names = []    # This list will be filled in self.async_refresh() with the column names
                                            # that represent specific privileges in the mysql.db table. These column
                                            # names are queried so that only the supported ones end up here.
        # Supported user global privilege list:
        self.global_privilege_names = []    # This list will be filled in self.async_refresh() with the column names
                                            # that represent specific privileges in the mysql.user table. These column
                                            # names are queried so that only the supported ones end up here.

        self.user_table_fields = []         # To be filled with all the fields in the mysql.user table.


    def get_valid_privileges(self):
        # self.global_privilege_names - column names from user table
        # PrivilegeInfo: map of user table's column name to privilege and its description
        privs = []
        for name in self.global_privilege_names:
            (priv, desc) = PrivilegeInfo.get(name, (None, None))
            if priv:
                privs.append(priv)

        return privs

    def async_refresh(self, callback):
        # Get the list of privileges supported by the version of MySQL Server we are connected to:
        if not self.schema_privilege_names:
            try:
                result = self.ctrl_be.exec_query("DESCRIBE mysql.db")
            except QueryError, e:
                if e.error == 1142:
                    raise PermissionDeniedError("Please make sure the used account has rights to the MySQL grant tables.\n%s" % e)
                raise e

            if result is not None:
                while result.nextRow():
                    field= result.stringByName("Field")
                    if field.endswith("_priv"):
                        self.schema_privilege_names.append(field)

        if not self.user_table_fields:
            try:
                result = self.ctrl_be.exec_query("DESCRIBE mysql.user")
            except QueryError, e:
                if e.error == 1142:
                    raise PermissionDeniedError("Please make sure the used account has rights to the MySQL grant tables.\n%s" % e)
                raise e

            if result is not None:
                while result.nextRow():
                    field= result.stringByName("Field")
                    self.user_table_fields.append(field)
                    if field.endswith("_priv"):
                        self.global_privilege_names.append(field)
                    elif field == "max_user_connections":
                        self.has_max_user_connections = True
                    elif field == "plugin":
                        self.has_plugin = True
                    elif field == "authentication_string":
                        self.has_authentication_string = True
                    elif field == "password_expired":
                        self.has_password_expired = True

        # get list of schema names
        schema_names = []
        try:
            result = self.ctrl_be.exec_query(LIST_SCHEMAS_QUERY)
        except QueryError, e:
            if e.error == 1142:
                raise PermissionDeniedError("Please make sure the used account has rights to the MySQL grant tables.\n%s" % e)
            raise e
        except Exception, e:
            raise Exception("Error querying privilege information: %s" % e)

        if result is not None:
            while result.nextRow():
                name = result.stringByName("Database")
                schema_names.append(name)

        schema_names.sort()
        self._schema_names = schema_names

        # Get a list of the account names from the mysql.user table:
        accounts = []
        try:
            result = self.ctrl_be.exec_query(LIST_ACCOUNTS_QUERY)
        except Exception, e:
            raise Exception("Error querying privilege information: %s" % e)

        if result:
            while result.nextRow():
                user = result.stringByName("User")
                host = result.stringByName("Host")
                accounts.append((user, host))


        # Get a list of invalid privileges
        def get_zombies(query, fields):
            try:
                result = self.ctrl_be.exec_query(query)
            except Exception, e:
                log_error("Could not get list of invalid privs: %s\nQuery: %s\n" % (e, query))
                return []
            privs = []
            while result and result.nextRow():
                user = result.stringByName("User")
                host = result.stringByName("Host")
                parts = [result.stringByName(f) for f in fields]
                privs.append(((user, host), ".".join(parts)))
            return privs

        # Sort list of accounts by User and Host
        # User is sorted alphabetically, Host is sorted by specificity in addition to alpha
        # (most specific items first)
        privs = get_zombies(ZOMBIE_SCHEMA_PRIVS_QUERY, ["Db"])
        privs += get_zombies(ZOMBIE_TABLE_PRIVS_QUERY, ["Db", "Table_name"])
        privs += get_zombies(ZOMBIE_COLUMN_PRIVS_QUERY, ["Db", "Table_name", "Column_name"])
        privs += get_zombies(ZOMBIE_PROCS_PRIVS_QUERY, ["Db", "Routine_name"])
        zombies = {}
        for account, priv in privs:
            if account not in zombies:
                zombies[account] = []
            zombies[account].append(priv)
        self._zombie_privs = zombies
        accounts += zombies.keys()

        accounts.sort(acl_compare)

        self._accounts = accounts
        self._account_info_cache = {}
        self._schema_privileges_by_user = {}

        callback()


    @property
    def schema_names(self):
        return self._schema_names

    @property
    def escaped_schema_names(self):
        return [escape_schema_name(s) for s in self._schema_names]

    @property
    def account_names(self):
        return self._accounts


    def do_delete_account(self, username, host):
        query = REMOVE_USER % {"user":escape_sql_string(username), "host":escape_sql_string(host)}
        try:
            self.ctrl_be.exec_sql("use mysql")
            self.ctrl_be.exec_sql(query)
        except QueryError, e:
            log_error('Error removing account %s@%s:\n%s' % (username, host, str(e)))
            if e.error == 1227: # MySQL error code 1227 (ER_SPECIFIC_ACCESS_DENIED_ERROR)
                raise Exception('Error removing the account  %s@%s:' % (username, host),
                                'You must have the global CREATE USER privilege or the DELETE privilege for the mysql '
                                'database')
            raise e

    def delete_account(self, account):
        if account.is_commited:
            self.do_delete_account(account.username, account.host)
        del self._account_info_cache[account.username+"@"+account.host]
        if (account.username, account.host) in self._accounts:
          self._accounts.remove((account.username, account.host))

    def revert_account(self, account, backup):
        try:
            i = self._accounts.index((account.username, account.host))
        except ValueError:
            pass
        else:
            self._accounts[i] = ((backup.username, backup.host))
            if account.username+"@"+account.host in self._account_info_cache:
                del self._account_info_cache[account.username+"@"+account.host]
        self._account_info_cache[backup.username+"@"+backup.host] = backup

        return backup


    def copy_account(self, account):
        copy = account.copy()
        copy.is_commited = False
        copy.username += '_copy'
        self._account_info_cache[copy.username+"@"+copy.host] = copy
        self._accounts.append((copy.username, copy.host))
        return copy


    def create_account(self):
        def unique_name(user, host, counter=None):
            name = user + ( str(counter) if counter else '' )
            if (name, host) in self._accounts:
                name = unique_name(user, host, counter+1 if isinstance(counter, int) else 1)
            return name

        acct = AdminAccount(self)
        acct.host = "%"
        acct.username = unique_name('newuser', acct.host)
        self._account_info_cache[acct.username+"@"+acct.host] = acct
        self._accounts.append((acct.username, acct.host))
        return acct


    def is_zombie(self, user, host):
        return self._zombie_privs.has_key((user,host))


    def get_zombie_privs(self, user, host):
        return self._zombie_privs.get((user,host), None)


    def async_get_account(self, callback, name, host):
        try:
          key = name+"@"+host

          if self._account_info_cache.has_key(key):
              callback(self._account_info_cache[key])
              return

          account = AdminAccount(self)
          account.load(name, host)

          self._account_info_cache[key] = account

          callback(account)
        except:
          mforms.Utilities.driver_shutdown()
          raise



    def revert_user_schema_privs(self, privs, backup):
        if privs.username in self._schema_privileges_cache:
            del self._schema_privileges_cache[privs.username]
        self._schema_privileges_cache[backup.username] = backup

        return backup




class AdminUserDbPrivEntry(object):
    db = None
    privileges = set()

    def __init__(self, db, privileges):
        assert type(privileges) == set
        self.db = db
        self.privileges = privileges


    def copy(self):
        return AdminUserDbPrivEntry(self.db, self.privileges.copy())




class AdminUserDbPrivs(object):
    _owner = None

    def __init__(self, owner):
        self._owner = owner
        self.entries = []
        self._deleted_entries = []

    @property
    def schema_privilege_names(self):
        return self._owner._owner.schema_privilege_names

    def copy(self):
        copy = AdminUserDbPrivs(self._owner)

        copy.entries = [e.copy() for e in self.entries]
        copy._deleted_entries = self._deleted_entries[:]

        return copy

    def add_entry(self, db, privileges):
        entry = AdminUserDbPrivEntry(db, privileges)
        self.entries.append(entry)
        return entry

    def del_entry(self, index):
        self._deleted_entries.append(self.entries[index])
        del self.entries[index]


    def load(self):
        # Schema privileges from Db table
        query = GET_ACCOUNT_SCHEMA_PRIVS_QUERY % {"user": escape_sql_string(self._owner.username), "host": escape_sql_string(self._owner.host)}
        try:
            result = self._owner.ctrl_be.exec_query(query)
        except Exception, e:
            raise Exception("Error querying security information: %s" % e)

        self.entries = []

        while result.nextRow():
            privs = set()
            for priv in self.schema_privilege_names:
                value = result.stringByName(priv)
                if value == 'Y':
                    privs.add(priv)

            schema = result.stringByName("Db")

            self.entries.append(AdminUserDbPrivEntry(schema, privs))

        self.entries.sort(lambda a, b: acl_compare(a.db, b.db))
        self._deleted_entries = []


    def save(self):
        # workaround for server bug
        self._owner.ctrl_be.exec_sql("use mysql")

        for deleted, entry in [(True, e) for e in self._deleted_entries] + [(False, e) for e in self.entries]:

            fields = { "user":escape_sql_string(self._owner.username),
                       "host":escape_sql_string(self._owner.host),
                       "db":entry.db
                     }

            granted_privs = []
            revoked_privs = []
            for priv in self.schema_privilege_names:
                (priv_name, description) = PrivilegeInfo.get(priv, (None,None))
                if not priv_name:
                    continue
                if (not deleted and priv in entry.privileges) or not entry.privileges:
                    granted_privs.append(priv_name)
                else:
                    revoked_privs.append(priv_name)
            if granted_privs and not deleted:
                fields['granted_privs'] = ', '.join(granted_privs)
                try:
                    self._owner.ctrl_be.exec_sql(GRANT_SCHEMA_PRIVILEGES_QUERY % fields)
                except QueryError, e:
                    if e.error in [1045, 1044]:
                        raise Exception('Error assigning privileges for %(user)s@%(host)s in schema %(db)s' % fields,
                                        'You must have the GRANT OPTION privilege, and you must have the privileges that you are granting')
                    raise e
            
            if revoked_privs:
                fields['revoked_privs'] = ', '.join(revoked_privs)
                try:
                    self._owner.ctrl_be.exec_sql(REVOKE_SCHEMA_PRIVILEGES_QUERY % fields)
                except QueryError, e:
                    if e.error in [1045, 1044]:
                        raise Exception('Error revoking privileges for %(user)s@%(host)s in schema %(db)s' % fields,
                                        'You must have the GRANT OPTION privilege, and you must have the privileges that you are revoking')
                    raise e


class AdminAccount(object):
    _owner = None

    _orig_username = None
    _orig_host = None
    _orig_password = None
    _orig_auth_string = None
    username = None
    password = None
    confirm_password = None
    password_expired = False
    host = None

    schema_privs = None

    max_questions = 0
    max_updates = 0
    max_connections = 0
    max_user_connections = 0
    auth_plugin = None
    auth_string = None
    old_authentication = False
    blank_password = False

    _global_privs = set()
    _custom_checked_privs = set()
    _remembered_custom_privs = set()
    _orig_global_privs = set()
    _orig_account_limits = {}

    is_commited = False # False means that account was not pushed to mysql server

    def __init__(self, owner):
        self._owner = owner
        self.schema_privs = AdminUserDbPrivs(self)

    @property
    def ctrl_be(self):
        return self._owner.ctrl_be


    def formatted_name(self):
        if self.host is not None:
            return "%s@%s" % (self.username, self.host)
        else:
            return self.username

    def copy(self):
        copy = AdminAccount(self._owner)
        copy.schema_privs = self.schema_privs.copy()
        copy.username = self.username
        copy.password = self.password
        copy.confirm_password = self.password
        copy.password_expired = self.password_expired
        copy.host = self.host
        copy.max_questions = self.max_questions
        copy.max_updates = self.max_updates
        copy.max_connections = self.max_connections
        copy.max_user_connections = self.max_user_connections
        copy.auth_plugin = self.auth_plugin
        copy.auth_string = self.auth_string
        copy.is_commited = self.is_commited
        copy._global_privs = self._global_privs.copy()
        copy._custom_checked_privs = self._custom_checked_privs.copy()
        return copy

    def snapshot_for_revert(self):
        copy = self.copy()
        copy._orig_username = self._orig_username
        copy._orig_password = self._orig_password
        copy._orig_auth_string = self._orig_auth_string
        copy._orig_host = self._orig_host
        return copy

    def toggle_priv(self, priv, flag):
        if flag:
            self._global_privs.update([priv])
        else:
            self._global_privs.difference_update([priv])
            self._remembered_custom_privs.difference_update([priv])

    @property
    def is_custom_role_needed(self):
        return bool(self._remembered_custom_privs or self._custom_checked_privs)

    def forget_custom_privs(self):
        self._remembered_custom_privs = set()

    def toggle_role(self, role, flag):
        def privs_for_role(role):
            if role == 'Custom':
                return self._remembered_custom_privs
            for rname, rdesc, rprivs in SecurityAdminRoles:
                if rname == role:
                    return set(rprivs).intersection(self._owner.global_privilege_names)
            return set()

        privs = privs_for_role(role)
        if privs:
            if flag:
                self._global_privs.update(privs)
            else:
                self._global_privs.difference_update(privs)

    @property
    def admin_roles(self):
        roles = []
        privs_from_roles = set()
        for rname, rdesc, rprivs in SecurityAdminRoles:
            # The role is active if the subset of its privileges that are supported by the current server
            # version are fully contained in the privileges that the user has:
            if set(rprivs).intersection(self._owner.global_privilege_names).issubset(self._global_privs):
                roles.append(rname)
                privs_from_roles.update(rprivs)
        self._custom_checked_privs = self._global_privs - privs_from_roles
        if self._custom_checked_privs:
            roles.append('Custom')
            self._remembered_custom_privs.update(self._custom_checked_privs)
        return roles


    @property
    def raw_privilege_names(self):
        return [PrivilegeInfo.get(p, [p])[0] for p in self._global_privs]


    def revoke_all(self):
        command = REVOKE_ALL % {"user":escape_sql_string(self.username),"host":self.host}
        try:
            self._owner.ctrl_be.exec_sql("use mysql")
            self._owner.ctrl_be.exec_sql(command)
        except QueryError, e:
            if e.error == 1227:
                raise Exception('Error revoking privileges for the account  %s@%s:' % (self.username, self.host),
                                'You must have the global CREATE USER privilege or the UPDATE privilege for the mysql '
                                'database')
            else:
                raise

    def expire_password(self):
        command = EXPIRE_PASSWORD % {"user":escape_sql_string(self.username),"host":self.host}
        try:
            self._owner.ctrl_be.exec_sql("use mysql")
            self._owner.ctrl_be.exec_sql(command)
        except QueryError, e:
            raise Exception('Error expiring password for account  %s@%s: %s' % (self.username, self.host, e))
        self.password_expired = True


    def save(self):
        queries = []
        if self.password != self.confirm_password:
            raise WBSecurityValidationError("The new password and its confirmation don't match. Please re-enter them.")

        # workaround for server bug with replication #14358854
        queries.append("use mysql")

        #if not self.username:
        #    raise WBSecurityValidationError("Username must not be blank")

        if not self.host:
            raise WBSecurityValidationError("Host name must not be blank")

        # check if username + host is duplicated
        if self.is_commited and (self.username != self._orig_username or self.host != self._orig_host):
            if (self.username, self.host) in self._owner.account_names:
                raise WBSecurityValidationError("The '%s' account already exists and cannot be saved." % (self.formatted_name()))
        elif not self.is_commited:
            if self._owner.account_names.count((self.username, self.host)) > 1:
                raise WBSecurityValidationError("The '%s' account already exists and cannot be saved." % (self.formatted_name()))

        fields = {
            "old_user" : escape_sql_string(self._orig_username) if self._orig_username else self._orig_username,
            "old_host" : escape_sql_string(self._orig_host) if self._orig_host else self._orig_host,
            "user" : escape_sql_string(self.username) or "NULL",
            "host" : escape_sql_string(self.host) or "",
            "password" : escape_sql_string(self.password or ""),
            "auth_plugin" : escape_sql_string(self.auth_plugin) if self.auth_plugin else None,
            "auth_string" : escape_sql_string(self.auth_string) if self.auth_string else None
        }
  
        password_already_set = False
        if not self.is_commited:  # This is a new account
            if self.auth_plugin and self.auth_plugin != 'mysql_native_password':
                if self.auth_string is None:
                    create_query = CREATE_USER_QUERY_PLUGIN
                else:
                    create_query = CREATE_USER_QUERY_PLUGIN_AUTH_STRING
            else:
                create_query = CREATE_USER_QUERY
                password_already_set = True
            queries[:0] = [ create_query % fields ]  # This query should be the first in the batch
                                                     # WARNING: Now the pwd is sent in clear text
        else:  # The account already exists

            assert self._orig_username is not None and self._orig_host is not None

            if self._orig_username != self.username or self._orig_host != self.host:  # Rename the user
                queries[:0] = [ RENAME_USER_QUERY % fields ]  # This query should be the first in the batch

        names = ["MAX_QUERIES_PER_HOUR", "MAX_UPDATES_PER_HOUR", "MAX_CONNECTIONS_PER_HOUR"] + (self._owner.has_max_user_connections and ["MAX_USER_CONNECTIONS"] or [])
        values = [str(s) for s in [self.max_questions, self.max_updates, self.max_connections] + (self._owner.has_max_user_connections and [self.max_user_connections] or [])]
        account_limits = dict(zip(names, values))
        limits_changed = account_limits != self._orig_account_limits

        is_normal_priv =  lambda priv: ( PrivilegeInfo.get(priv, (None,None))[0] and 
                                         PrivilegeInfo.get(priv, (None,None))[0][0] != '*'
                                       )

        all_normal_privs = set( priv for priv in self._owner.global_privilege_names if is_normal_priv(priv) )
        new_granted_privs = (self._global_privs - self._orig_global_privs) & all_normal_privs
        orig_revoked_privs =  all_normal_privs - self._orig_global_privs
        new_revoked_privs = all_normal_privs - self._global_privs - orig_revoked_privs

        if new_granted_privs or limits_changed:
            if 'Grant_priv' in new_granted_privs:
                account_limits['GRANT'] = 'OPTION'
                new_granted_privs.remove('Grant_priv')
            if (all_normal_privs - new_granted_privs) <= set(['Grant_priv']):
                priv_list = ['ALL PRIVILEGES']
            else:
                priv_list = [ PrivilegeInfo[priv][0] for priv in new_granted_privs ]
            fields['granted_privs'] = ', '.join(priv_list) or 'USAGE'
            grant_query = GRANT_GLOBAL_PRIVILEGES_QUERY % fields
            with_clause = ''
            for key, value in account_limits.iteritems(): #name, value in zip(names, values):
                if value != self._orig_account_limits.get(key):
                    if not with_clause:
                        with_clause = ' WITH '
                    with_clause += "%s %s "%(key, value)
            queries.append(grant_query + with_clause)

        if new_revoked_privs:
            if all_normal_privs - new_revoked_privs: #set(self._owner.global_privilege_names) - revoked_privs_set:  # Revoke a subset of all privs
                priv_list = [ PrivilegeInfo[priv][0] for priv in new_revoked_privs ]
                fields['revoked_privs'] = ', '.join(priv_list)
                queries.append(REVOKE_GLOBAL_PRIVILEGES_QUERY % fields)
            else:  # All privs are to be revoked so use the revoke all query
                queries.append(REVOKE_ALL % fields)

        if self.password != self._orig_password and not password_already_set:
            change_pw = CHANGE_PASSWORD_QUERY if self._owner.ctrl_be.target_version and self._owner.ctrl_be.target_version < Version(5,7,6) else CHANGE_PASSWORD_QUERY_576
            blank_pw = BLANK_PASSWORD_QUERY if self._owner.ctrl_be.target_version and self._owner.ctrl_be.target_version < Version(5,7,6) else BLANK_PASSWORD_QUERY
            # special hack required by server to handle sha256 password accounts
            if self.auth_plugin == "sha256_password":
                queries.append("SET old_passwords = 2")
            else:
                queries.append("SET old_passwords = 0")
            if fields["password"]:
                queries.append(change_pw % fields)
            else:
                queries.append(blank_pw % fields)

        action = "changing" if self.is_commited else "creating"
        for query in queries:
            try:
                self._owner.ctrl_be.exec_sql(query)
            except QueryError, e:
                if e.error == 1142:
                    raise Exception("Error %s account %s@%s: Insufficient rights to perform operation"%(action, self.username, self.host))
                else:
                    raise Exception("Error %s account %s@%s: %s"%(action, self.username, self.host, e.errortext or e))
            except Exception, e:
                raise Exception("Error %s account %s@%s: %s"%(action, self.username, self.host, e))


        new_attrs = set([p for p in self._global_privs if p.endswith("_attr")])
        old_attrs = set([p for p in self._orig_global_privs if p.endswith("_attr")])


        def grant_special(priv):
            db, tables, privs, grant, revoke = AdminAttributes[priv]
            if "%(table)s" in grant:
                for table in tables:
                    query = grant % {"table":table, "user":self.username, "host":self.host}
                    self._owner.ctrl_be.exec_sql(query)
            else:
                query = grant % {"table":table, "user":self.username, "host":self.host}
                self._owner.ctrl_be.exec_sql(query)

        def revoke_special(priv):
            db, tables, privs, grant, revoke = AdminAttributes[priv]
            if "%(table)s" in revoke:
                for table in tables:
                    query = revoke % {"table":table, "user":self.username, "host":self.host}
                    self._owner.ctrl_be.exec_sql(query)
            else:
                query = grant % {"table":table, "user":self.username, "host":self.host}
                self._owner.ctrl_be.exec_sql(query)

        # check for newly granted special privs
        for priv in new_attrs.difference(old_attrs):
            grant_special(priv)

        # check for revoked special privs
        for priv in old_attrs.difference(new_attrs):
            revoke_special(priv)

        self.is_commited = True

        self.schema_privs.save()



    def load(self, username, hostname):
        self.is_commited = True
        # Basic stuff from User table
        query = GET_ACCOUNT_QUERY % {"user":escape_sql_string(username),"host":escape_sql_string(hostname)}
        try:
            result = self._owner.ctrl_be.exec_query(query)
        except Exception, e:
            raise Exception("Error querying security information: %s" % e)

        if not result.nextRow():
            raise Exception("Could not load account information for %s@%s"%(username,hostname))

        self.username = result.stringByName("User")
        self.host = result.stringByName("Host")

        self._orig_password = "UnchangedPassword\t"
        self.password = self._orig_password
        self._orig_username = self.username
        self._orig_host = self.host
        self._orig_auth_string = self.auth_string

        self.max_questions = result.intByName("max_questions")
        self.max_updates = result.intByName("max_updates")
        self.max_connections = result.intByName("max_connections")
        self._orig_account_limits = {
            "MAX_QUERIES_PER_HOUR"      : str(self.max_questions),
            "MAX_UPDATES_PER_HOUR"      : str(self.max_updates),
            "MAX_CONNECTIONS_PER_HOUR"  : str(self.max_connections),
                                    }
        if self._owner.has_max_user_connections:
            self.max_user_connections = result.intByName("max_user_connections")
            self._orig_account_limits["MAX_USER_CONNECTIONS"] = str(self.max_user_connections)
        if self._owner.has_plugin:
            self.auth_plugin = result.stringByName("plugin")
        if self._owner.has_authentication_string:
            self.auth_string = result.stringByName("authentication_string")
        self.password_expired = False
        if self._owner.has_password_expired:
            self.password_expired = result.stringByName("password_expired") == 'Y'

        password_column = 'password' if self._owner.ctrl_be.target_version and self._owner.ctrl_be.target_version < Version(5,7,6) else 'authentication_string'
        self.old_authentication = len(result.stringByName(password_column)) == 16
        self.blank_password = len(result.stringByName(password_column)) == 0

        self._global_privs = set()
        for priv in self._owner.global_privilege_names:
            if result.stringByName(priv) == 'Y':
                self._global_privs.add(priv)

        self.forget_custom_privs()

        """ not necessary, IS is accessible to all
        # privs from information_schema tables
        query = GET_ACCOUNT_IS_TABLE_PRIVS_QUERY % {"user":username,"host":hostname}
        result = modules.DbMySQLQuery.executeQuery(self._owner._connection, query)
        if result < 0:
            raise Exception("Error querying information_schema table: "+modules.DbMySQLQuery.lastError())

        try:
            is_privs = []
            while modules.DbMySQLQuery.resultNextRow(result):
                table = result.stringByName("Table_name")
                table_privs = result.stringByName("Table_priv")
                is_privs.append((table, table_priv and table_privs.split(",") or []))
        finally:
            modules.DbMySQLQuery.closeResult(result)
        """

        # privs from mysql tables
        query = GET_ACCOUNT_MYSQL_TABLE_PRIVS_QUERY % {"user":escape_sql_string(username),"host":escape_sql_string(hostname)}
        try:
            result = self._owner.ctrl_be.exec_query(query)
        except Exception, e:
            raise Exception("Error querying mysql table: %s" % e)

        mysql_privs = {}
        while result.nextRow():
            table = result.stringByName("Table_name")
            table_privs = result.stringByName("Table_priv")
            mysql_privs[table] = table_privs and table_privs.split(",") or []

        # interpret the privileges
        for name, (db, tables, required_privs, grant, revoke) in AdminAttributes.items():
            if db == "mysql":
                ok = True
                for table in tables:
                    if not mysql_privs.has_key(table):
                        ok = False
                        break
                    if not set(required_privs).issubset(set(mysql_privs[table])):
                        ok= False
                        break
                if ok:
                    self._global_privs.add(name)

        self._orig_global_privs = self._global_privs.copy()

        self.schema_privs.load()

    def upgrade_password_format(self):
        queries = []
        if self.password != self.confirm_password:
            raise WBSecurityValidationError("The new password and its confirmation don't match. Please re-enter them.")

        queries.append("use mysql")

        if not self.host:
            raise WBSecurityValidationError("Host name must not be blank")

        fields = {
            "user" : escape_sql_string(self.username) or "NULL",
            "host" : escape_sql_string(self.host) or "",
            "password" : escape_sql_string(self.password or ""),
        }

        queries.append("SET old_passwords = 0")        
        if self._owner.ctrl_be.target_version and self._owner.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 5, 7):
            queries.append("UPDATE mysql.user SET plugin = 'mysql_native_password' WHERE user = '%(user)s' AND host = '%(host)s'" % fields)
        queries.append("FLUSH PRIVILEGES")
        change_pw = CHANGE_PASSWORD_QUERY if self._owner.ctrl_be.target_version and self._owner.ctrl_be.target_version < Version(5,7,6) else CHANGE_PASSWORD_QUERY_576
        queries.append(change_pw % fields)
        queries.append("FLUSH PRIVILEGES")
        
        action = "changing"
        for query in queries:
            try:
                self._owner.ctrl_be.exec_sql(query)
            except QueryError, e:
                if e.error == 1142:
                    raise Exception("Error %s account %s@%s: Insufficient rights to perform operation"%(action, self.username, self.host))
                else:
                    raise Exception("Error %s account %s@%s: %s"%(action, self.username, self.host, e.errortext or e))
            except Exception, e:
                raise Exception("Error %s account %s@%s: %s"%(action, self.username, self.host, e))
