# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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

""" This module implements a manager for loading the correct DB API 2.0 
compliant driver from a db_mgmt_Connection object and  interface for the
underlying drivers. This module itself complies with DB API 2.0.
"""

import grt
from workbench.utils import replace_string_parameters, parameters_from_dsn, dsn_parameters_to_connection_parameters

def get_connection_parameters(conn, do_not_transform=False):
    provided_params = dict(conn.parameterValues)

    # Start filling the all_params dict with the predefined values from the driver data:
    all_params = dict( (param.name, param.defaultValue) for param in conn.driver.parameters )
    all_params.update(provided_params)
    if all_params.get('dsn', ''):  # If dsn is provided, get parameter values from there
        try:
            all_params.update(dsn_parameters_to_connection_parameters(parameters_from_dsn(all_params['dsn'])))
        except Exception:
            pass
    if do_not_transform:
        return all_params
    
    conn_params = { 'hostName'  : '127.0.0.1',
                    'port'      : '1433',
                    'schema'    : '',
                    'userName'  : '',
                    'driver'    : '',
                    'dsn'       : '',
                    'password'  : '',
                    'extras'    : '',
                  }
    conn_params.update(all_params)
    return conn_params


def get_odbc_connection_string(conn, password):
    conn_params = get_connection_parameters(conn)
    if password is not None:
        conn_params['password'] = password  # TODO: Check password handling policy: When should we take this or the one in the parameters?
    connection_string_template = conn.driver.connectionStringTemplate or 'DRIVER={%driver%};SERVER=%hostName%;PORT=%port%;DATABASE={%schema%};UID=%userName%;PWD={%password%}'
    connection_string = replace_string_parameters(connection_string_template, conn_params)
    return connection_string


def is_odbc_connection(conn):
    return conn.driver.driverLibraryName == "pyodbc"


def connect(conn, password=''):
    """ Establish a connection to a database and return a Python DB API 2.0 connection object.
    
    :param conn:      An instance of :class:`db_mgmt_Connection` that contains the needed parameters
                      to set the connection up. You must ensure that this object has a :attr:`driver`
                      attribute with a :attr:`driverLibraryName` attribute that specifies a python module
                      name that will be imported and its :meth:`connect` method called to actually perform
                      the connection.

    :type conn: db_mgmt_Connection

    :param password:  A password to authenticate the user specified in :attr:`conn` with (optional).

    :type password: string

    :returns: A Python DB API 2.0 connection object that can be used to communicate to the target RDBMS.
    """

    connection_string = get_odbc_connection_string(conn, password)
    import re
    connection_string_fixed = re.sub("(.*PWD=)([^;]*)(.*)", r"\1XXXX\3", connection_string)
    connection_string_fixed = re.sub("(.*PASSWORD=)([^;]*)(.*)", r"\1XXXX\3", connection_string_fixed)
    grt.send_info('Opening ODBC connection to %s...' % connection_string_fixed)

    library = __import__(conn.driver.driverLibraryName, globals(), locals())
    connection = library.connect(connection_string, password=password)

    return connection 
    

    
