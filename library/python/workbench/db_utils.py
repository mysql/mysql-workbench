# Copyright (c) 2010, 2016, Oracle and/or its affiliates. All rights reserved.
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

from grt import modules
from grt import DBLoginError

def escape_sql_string(s):
    return s.replace("\\", "\\\\").replace("'", "\\'")

def escape_sql_identifier(s):
    return s.replace("`", "``")


def strip_password(s):
    import re
    while True:
        ss = re.sub(r"(.*PASSWORD.*\(')(.*)('\).*)", r"\1XXXXXX\3", s)
        if s == ss:
            break
        s = ss
    return s

    
def substring_to_delimiter(source, index, limit, force_limit = False):
    """
        Extracts from a string starting at the given index and ending
        once the first character in limit is found.
        
        If force_limit is True, will return None if the limit is not found
        and the end of the string is reached.
    """

    # When limit is quoting handles special quote embedding cases:
    # - Escaped quoting using \
    # - Quoting using the quote character twice: '', "" or ``
    quoting = '\'"`'
    handle_embedded_quoting = True if limit in quoting else False
    

    # Sweeps the string starting at index and until a character in limit is found
    token = ''
    limit_found = False
    escape_found = False

    for char in source[index:]:
        if handle_embedded_quoting:
            if char == '\\' and not escape_found:
                escape_found = True
                token += char
                continue
                
        if char in limit:
            if not escape_found:
                if not limit_found:
                    limit_found = True
                else:
                    token += char
                    limit_found = False
        else:
            if limit_found:
                break
            
        if not limit_found:
            escape_found = False
            token += char

    # Limit may be mandatory or not, returns None if
    # it was not found
    if force_limit and not limit_found:
        return None
    else:
        return token

def parse_mysql_ids(source):
    """
        Extracts from a string an array with all the valid IDs found
        Expected format is a dot separated list of IDs where they could optionally be quoted
        by single, double or back quotes.

        If an invalid ID is found the process will stop.
    """
    ids = []
    index = 0;
    length = len(source)
    previous_token = False
    
    # Sweeps a string extracting all the 
    while index < length:
        token = ''
        char = source[index]

        # In case of quoting found, the ID will be extracted until the closing quote is found
        if char in '"\'`':
            token = substring_to_delimiter(source, index + 1, char, True)
            if token:
                index = index + len(token) + 2
        else:
            # The dot as separator is just skipped as long as previos was a valid token
            # i.e. 2 in a row is a mistake
            if char in '. ':
                if previous_token:
                    index += 1
                    previous_token = False
                else:
                    token = None

            else:
                # If no quoting is found, next ID will be until the delimiters are found
                token = substring_to_delimiter(source, index, '. ')
                index = index + len(token)

        if token:
            ids.append(token)
            previous_token = True
        elif token is None:
            break

    return ids   

class MySQLError(Exception):
    def __init__(self, msg, code, location):
        Exception.__init__(self, msg + " (code %i)"%code)
        self.code = code
        self.location = location


class MySQLLoginError(MySQLError, DBLoginError):
    def __init__(self, msg, code, location):
        MySQLError.__init__(self, msg, code, location)
        DBLoginError.__init__(self, msg)


class QueryError(Exception):
  not_connected_errors = (2006, 2013, 2026, 2055, 2048)
  def __init__(self, msg, error, errortext = None):
    self.msg = msg
    self.error = error
    self.errortext = errortext

  def __str__(self):
    return self.msg + ".\nSQL Error: " + str(self.error)

  def is_connection_error(self):
    code = 0
    try:
      code = int(self.error)
    except ValueError:
      pass
    return code in self.not_connected_errors

  def is_error_recoverable(self):
    return self.error != 2006 # Probably add more errors here


class ConnectionTunnel:
    def __init__(self, info):
        self.tunnel = modules.DbMySQLQuery.openTunnel(info)
        if self.tunnel > 0:
            self.port = modules.DbMySQLQuery.getTunnelPort(self.tunnel)
        else:
            self.port = None
    
    def __del__(self):
        if self.tunnel > 0:
            modules.DbMySQLQuery.closeTunnel(self.tunnel)


class MySQLResult:
    def __init__(self, result):
        self.result = result


    def __del__(self):
        if self.result:
            modules.DbMySQLQuery.closeResult(self.result)

    def firstRow(self):
        return modules.DbMySQLQuery.resultFirstRow(self.result)

    def nextRow(self):
        return modules.DbMySQLQuery.resultNextRow(self.result)

    
    def stringByName(self, name):
        return modules.DbMySQLQuery.resultFieldStringValueByName(self.result, name)

    def unicodeByName(self, name):
        s = modules.DbMySQLQuery.resultFieldStringValueByName(self.result, name)
        if type(s) is str:
            return s.decode("utf-8")
        return s

    def intByName(self, name):
        return modules.DbMySQLQuery.resultFieldIntValueByName(self.result, name)

    def floatByName(self, name):
        return modules.DbMySQLQuery.resultFieldDoubleValueByName(self.result, name)

    def stringByIndex(self, i):
        return modules.DbMySQLQuery.resultFieldStringValue(self.result, i)

    def unicodeByIndex(self, i):
        s = modules.DbMySQLQuery.resultFieldStringValue(self.result, i)
        if type(s) is str:
            return s.decode("utf-8")
        return s

    def floatByIndex(self, i):
        return modules.DbMySQLQuery.resultFieldDoubleValue(self.result, i)

    def intByIndex(self, i):
        return modules.DbMySQLQuery.resultFieldIntValue(self.result, i)

    def numFields(self):
        return modules.DbMySQLQuery.resultNumFields(self.result)

    def numRows(self):
        return modules.DbMySQLQuery.resultNumRows(self.result)

    def fieldName(self, i):
        return modules.DbMySQLQuery.resultFieldName(self.result, i)

    def fieldType(self, i):
        return modules.DbMySQLQuery.resultFieldType(self.result, i)
        

class MySQLConnection:
    """
        Connection to a MySQL server, use as:
          info = grt.root.wb.rdbmsMgmt.storedConns[0]
          conn = MySQLConnection(info)
          conn.connect()
          result = conn.executeQuery("SHOW DATABASES")
          flag = result.firstRow()
          while flag:
              print result.stringByName("Database")
              flag = result.nextRow()
    """
    def __init__(self, info, status_cb = None, password = None):
        assert type(status_cb) is not unicode
        self.connect_info = info
        self.connection = 0
        self.server_down = 0
        self.status_cb = status_cb
        self.password = password


    def __del__(self):
        self.disconnect()

    def send_status(self, code, error = None):
        if self.status_cb:
            self.status_cb(code, error, self.connect_info)

    def connect(self):
        self.server_down = False
        if not self.connection:
            params = self.connect_info.parameterValues
            old_timeout_value = None
            if params.has_key('OPT_READ_TIMEOUT'):
                old_timeout_value = params['OPT_READ_TIMEOUT']
            params['OPT_READ_TIMEOUT'] = 5*60

            #self.thread = thread.get_ident()
            if self.password is not None:
                self.connection = modules.DbMySQLQuery.openConnectionP(self.connect_info, self.password)
            else:
                self.connection = modules.DbMySQLQuery.openConnection(self.connect_info)

            if old_timeout_value:
                params['OPT_READ_TIMEOUT'] = old_timeout_value
            else:
                del params['OPT_READ_TIMEOUT']
            if self.connection < 0:
                self.connection = 0
                code = modules.DbMySQLQuery.lastErrorCode()
                if code == 1045:
                    raise MySQLLoginError(modules.DbMySQLQuery.lastError(), modules.DbMySQLQuery.lastErrorCode(), "%s@%s" % (self.connect_info.parameterValues["userName"], self.connect_info.parameterValues["hostName"]))
                    
                if code in (2003,2002):
                    self.server_down = True
                raise MySQLError(modules.DbMySQLQuery.lastError(), modules.DbMySQLQuery.lastErrorCode(), "%s@%s" % (self.connect_info.parameterValues["userName"], self.connect_info.parameterValues["hostName"]))

            self.send_status(0, "Connection created")

    def ping(self):
        self.executeQuery("SELECT 1")
        return True

    def try_ping(self):
        if not self.is_connected():
            return False
        try:
            self.sql.exec_query("select 1")
        except QueryError, e:
            return False
        return True
      
    def disconnect(self):
        if self.connection:
            if hasattr(modules.DbMySQLQuery, 'closeConnection'): 
                modules.DbMySQLQuery.closeConnection(self.connection)
            self.connection = 0
            self.send_status(-1, "Connection closed by client")
    
    @property
    def is_connected(self):
        return self.connection > 0
    

    def execute(self, query):
        if self.connection:
            #assert self.thread == thread.get_ident()
            result = modules.DbMySQLQuery.execute(self.connection, query)
            if result < 0:
              code = modules.DbMySQLQuery.lastConnectionErrorCode(self.connection)
              error = strip_password(modules.DbMySQLQuery.lastConnectionError(self.connection))
              self.send_status(code, error)
              raise QueryError("Error executing '%s'\n%s" % (strip_password(query), error), code, error)

            self.send_status(0)
            return result == 0
        else:
            self.send_status(-1, "Connection to MySQL server is currently not established")
            raise QueryError("Connection to MySQL server is currently not established", -1)


    def executeQuery(self, query):
        if self.connection:
            #assert self.thread == thread.get_ident()
            result = modules.DbMySQLQuery.executeQuery(self.connection, query.encode("utf-8") if type(query) is unicode else query)
            if result < 0:
                code = modules.DbMySQLQuery.lastConnectionErrorCode(self.connection)
                error = modules.DbMySQLQuery.lastConnectionError(self.connection)
                self.send_status(code, error)
                raise QueryError("Error executing '%s'\n%s"%(query, error), code, error)

            self.send_status(0)
            return MySQLResult(result)
        else:
            self.send_status(-1, "Connection to MySQL server is currently not established")
            raise QueryError("Connection to MySQL server is currently not established", -1)

    def executeQueryMultiResult(self, query):
        if self.connection:
            result = modules.DbMySQLQuery.executeQueryMultiResult(self.connection, query.encode("utf-8") if type(query) is unicode else query)
            if len(result) == 0:
                code = modules.DbMySQLQuery.lastConnectionErrorCode(self.connection)
                error = modules.DbMySQLQuery.lastConnectionError(self.connection)
                self.send_status(code, error)
                raise QueryError("Error executing '%s'\n%s"%(query, error), code, error)

            self.send_status(0)

            result_list = []

            for index in range(0, len(result)):
                result_list.append(MySQLResult(result[index]))
            return result_list
        else:
            self.send_status(-1, "Connection to MySQL server is currently not established")
            raise QueryError("Connection to MySQL server is currently not established", -1)

    def updateCount(self):
        return modules.DbMySQLQuery.lastUpdateCount(self.connection)
    affectedRows = updateCount

