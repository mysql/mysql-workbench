# Copyright (c) 2013, 2016 Oracle and/or its affiliates. All rights reserved.
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

import grt
from workbench.log import log_warning, log_error
from workbench.db_utils import parse_mysql_ids


class PrivilegeTarget(object):
    """
      This class holds the information of the priv_level item at the
      GRANT statement.

      It will be used not only to hold the information but also to provide functions
      for the matching against other priv_level definitions.
    """
    def __init__(self):
        self.schema = "*"
        self.object = "*"
        self.object_type = "*"

    def set_from_string(self, data):
        if data != '*' and data != '*.*':
          values = parse_mysql_ids(data)

          if len(values) == 2:
              self.schema, self.object = tuple(values)
          else:
              self.object = data

    def identical(self, target_string):
        return target_string == '%s.%s' % (self.schema, self.object)

    def matches(self, other):
        """ 
          There are three scope types considered:
          - Global: schema and table are *
          - Schema: schema is *
          - Table: Both are not *
        """
        ret_val = False

        # Global privilege matches any target definition
        if self.schema == '*' and self.object == '*':
            ret_val = True
        elif self.schema == other.schema:
            if self.object == '*':
                ret_val = True
            else:
                ret_val = (self.object == other.object)

        return ret_val


    def get_target_type(self):
        """
          Returns the privilege level based on the data.
          Note: COLUMN is not considered at this point.
        """
        ret_val = ''
        if self.schema == '*' and self.object == '*':
            ret_val = 'GLOBAL'
        elif self.object != '*':
            ret_val = 'OBJECT'
        else:
            ret_val = 'DATABASE'

        return ret_val



class UserHostPrivileges(object):
    """
      This class is intended to load the privileges applicable to a user/host
      pair. Once loaded it will be able to verify if specific privileges are
      granted to the user or not.

      Initialization parameters:
      - user: the username for which the privileges will be verified.
      - host: the real hostname from which the user will be connecting to the
              database. (No wildcard should be used).
      - context: needed to query the data from the database.
    """
    def __init__(self, user, host, context):
        self.user = user
        self.host = host
        self.context = context
        

        # Creates the privilege lists available at the different levels
        self._define_privilege_tables()

        # Will hold the configured hosts that apply to the target host
        self.applicant_hosts = []

        # Will be used to accummulate the privileges applicable to the user/host
        # combination.
        # The format is a key/value pair where key is the target definition
        # And the key is the binary representation of the granted privileges
        # on that target.
        self._granted_privileges = {}

        self._character_sets = self.context.ctrl_be.editor.connection.driver.owner.characterSets
        self._target_version = self.context.ctrl_be.editor.serverVersion
        # Sets the server version as needed for the parser
        version = self.context.ctrl_be.target_version
        if version:
            self._server_version = version.majorNumber * 10000 + version.minorNumber * 100 + version.releaseNumber
        else:
            self._server_version = 0


    def _define_privilege_tables(self):
        """
          This method is used just to create the privilege lists
          at the different levels available in MySQL
        """
        self._privileges = {}
        self._privileges['COLUMN'] = ['INSERT','SELECT', 'UPDATE']

        self._privileges['OBJECT'] = []
        self._privileges['OBJECT'].extend(self._privileges['COLUMN'])
        self._privileges['OBJECT'].extend(['ALTER', 'ALTER ROUTINE', 'CREATE', 'CREATE ROUTINE', 'CREATE VIEW', 'DELETE', 'DROP', 'EXECUTE', 'GRANT OPTION', 'INDEX', 'SHOW VIEW'])

        self._privileges['DATABASE'] = []
        self._privileges['DATABASE'].extend(self._privileges['OBJECT'])
        self._privileges['DATABASE'].extend(['CREATE TEMPORARY TABLES', 'LOCK TABLES'])

        self._privileges['GLOBAL'] = []
        self._privileges['GLOBAL'].extend(self._privileges['DATABASE'])
        self._privileges['GLOBAL'].extend(['FILE', 'PROCESS', 'RELOAD', 'REPLICATION CLIENT', 'REPLICATION SLAVE', 'SHOW DATABASES', 'SHUTDOWN', 'SUPER', 'CREATE USER'])

    def get_privilege_value(self, privileges, level):
        """
          This function will return a binary value indicating the set of privileges
          at the indicated level.
        """
        ret_val = 0

        if self._privileges.has_key(level):
            new_value = 0
            
            # Gets the value of each privilege in privileges and appends its
            # value to the binary representation of all the privileges
            for priv in privileges:
                try:
                  index = self._privileges[level].index(priv)
                  new_value = 2 ** index;
                except ValueError:
                  if priv == 'ALL' or priv == 'ALL PRIVILEGES':
                      new_value = (2 ** len(self._privileges[level])) - 1
                      new_value -= self.get_privilege_value(['GRANT OPTION'], 'OBJECT')

                
                ret_val |= new_value

        return ret_val

    def add_privileges(self, target, priv_list):

        # Converts the privilege lists to its binary representation
        priv_value = self.get_privilege_value(priv_list, target.get_target_type())

        # Creates the entry for the target if it doesn't exist
        if not self._granted_privileges.has_key(target):
            self._granted_privileges[target] = 0

        # Appends the new privileges to the existing ones
        self._granted_privileges[target] |= priv_value


    def check_privileges(self, target, privileges):
        for tgt in self._granted_privileges.keys():
            if tgt.matches(target):
                # Gets the included privileges
                includes = self._granted_privileges[tgt] & privileges

                # Unsets the already included privileges
                privileges -= includes

        # Returns the remaining privileges
        return privileges

    def load_hosts(self):
        """
        Will identify the host in the database that are applicable to a given host
        (Host in database can be defined with wildcard so it is not a straight comparison)
        """
        hosts = None

        # note the comparison of the host field is backwards than normal.
        # this in on purpose to allow identifying using SQL all the hosts in the grant definitions
        # that do match the host being queried.
        result = self.context.ctrl_be.exec_query("""SELECT host 
                                                    FROM mysql.user 
                                                    WHERE user = '%s' AND '%s' LIKE host""" % (self.user, self.host))
        if result:
            hosts = []
            while result.nextRow():
                hosts.append(result.stringByIndex(1))
        else:
            log_warning('There are no grants defined for the user %s on hosts matching %s\n' % (self.user, self.host))

        self.applicant_hosts = hosts

        return hosts

    def load_privileges(self, host):
      
        # Clears any previously loaded privileges
        self._granted_privileges = {}
        
        if host in self.applicant_hosts:
            # If there are hosts it means there are privileges applicable for the user
            # On the indicated host
            result = self.context.ctrl_be.exec_query("SHOW GRANTS FOR `%s`@`%s`" % (self.user, host))
            
            context = grt.modules.MySQLParserServices.createParserContext(self._character_sets, self._target_version, 'STRICT_TRANS_TABLES,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION', 1)
            
            if result:
                while result.nextRow():
                    statement = result.stringByIndex(1)
                    grant_data = None
                    
                    try:
                        grant_data = grt.modules.MySQLParserServices.parseStatementDetails(context, statement)
                        if not grant_data['error']:
                            # Gets the target scope for the privileges
                            target_string = grant_data['target']
            
                            target = None
            
                            # Search for an already existing target
                            for tgt in self._granted_privileges.keys():
                                if tgt.identical(target_string):
                                    target = tgt
            
                            # If not found, creates one
                            if not target:
                                target = PrivilegeTarget()
                                target.set_from_string(target_string)
            
                            # Gets the privilege list
                            priv_list = grant_data['privileges']
            
                            # Adds the privileges to the granted list
                            self.add_privileges(target, priv_list)
                        else:
                            log_error('An error occurred parsing GRANT statement: %s\n -> %s\n' % (statement, grant_data['error']))
                    except Exception, exc:
                        log_error('An error occurred parsing GRANT statement: %s\n -> %s\n' % (statement, exc))

            else:
                log_warning('There are no grants defined for %s@%s\n' % (self.user, self.host))


    def includes_privileges(self, target_str, privileges):
        target = PrivilegeTarget()
        target.set_from_string(target_str)
        priv_val = self.get_privilege_value(privileges, target.get_target_type())

        return self.check_privileges(target, priv_val) == 0

