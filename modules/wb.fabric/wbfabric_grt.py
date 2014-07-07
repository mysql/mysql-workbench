# -*- coding: utf-8 -*-
# MySQL Workbench module
# <description>
# Written in MySQL Workbench 6.2.0

from wb import *
import grt
from grt import log_warning
import mforms
import urllib2
import traceback

from wb_common import OperationCancelledError

fabric_unavailable_message = ""

try: 
    from mysql.fabric.config import Config
    from mysql.fabric.command import get_groups, get_command, get_commands
    from mysql.fabric.services import find_client, find_commands
    from mysql.fabric.options import OptionParser
except ImportError, e:
    log_warning("WBFabric Module", "Error loading MySQL Fabric components.\nMySQL Fabric support is unavailable: %s\n" % traceback.format_exc())

    if str(e).find('No module named mysql.fabric') == 0:
        fabric_unavailable_message = "MySQL Fabric support not available.\nPlease make sure MySQL Utilities version 1.4.3 is installed to use this feature.\nSee log file for more details."
    else:
        fabric_unavailable_message = "A component needed by this version of MySQL Fabric could not be found.\nPlease refer to the MySQL Workbench Release Notes for details about supported MySQL Fabric versions and setups."
        log_warning("WBFabric Module", "Error loading MySQL Fabric components.\nMySQL Fabric support is unavailable: %s\n" % traceback.format_exc())
except Exception, e:
    log_warning("WBFabric Module", "Error loading MySQL Fabric components.\nMySQL Fabric support is unavailable: %s\n" % traceback.format_exc())
    fabric_unavailable_message = "MySQL Fabric support not available.\n Error : %s" % str(e)

ModuleInfo = DefineModule(name= "WBFabric", author= "Oracle Corp.", version="1.0")

# ----------------------------------------------------------------------
# Patch for python 2.7 which does not have getheader method for urlinfo
# object
# ----------------------------------------------------------------------

class addinfourl(urllib2.addinfourl):
    """
    Replacement addinfourl class compatible with python-2.7's xmlrpclib

    In python-2.7, xmlrpclib expects that the response object that it receives
    has a getheader method. httplib.HTTPResponse provides this but
    urllib2.addinfourl does not. Add the necessary functions here, ported to
    use the internal data structures of addinfourl.
    """
    def getheader(self, name, default=None):
        if self.headers is None:
            raise httplib.ResponseNotReady()
        return self.headers.getheader(name, default)

    def getheaders(self):
        if self.headers is None:
            raise httplib.ResponseNotReady()
        return self.headers.items()

urllib2.addinfourl = addinfourl
# ----------------------------------------------------------------------


def create_config(conn):
  # Retrieves the fabric node connection data
  host = conn.parameterValues["hostName"]
  port = conn.parameterValues["port"]
  user = conn.parameterValues["userName"]
  
  config = None
  
  accepted, password = mforms.Utilities.find_or_ask_for_password("Fabric Node Connection", '%s@%s' % (host, port), user, False)
  if accepted:
      config = Config(None, {'protocol.xmlrpc':{'address':'%s:%s' % (host, port), 'user':user, 'password':password, 'realm':'MySQL Fabric'}})
  else:
      raise OperationCancelledError("Password input cancelled")
  
  return config


# ----------------------------------------------------------------------
# The code in this section has been pulled from the mysqlfabric utility
# and tweaked
# ----------------------------------------------------------------------
def create_command(group_name, command_name, options, args, config):
    """Create command object.
      """
    options = None
    
    try:
        # Registers the commands
        find_commands()

        # Fetch command class and create the command instance.
        command = get_command(group_name, command_name)()
        command.command_options=[]
        
        # Set up options for command
        opt_parser = OptionParser()
        
        command.add_options(opt_parser)
        
        # Parse arguments
        options, args = opt_parser.parse_args(args, options)
        
        # Create a protocol client for dispatching the command and set
        # up the client-side information for the command. Inside a
        # shell, this only have to be done once, but here we need to
        # set up the client-side of the command each time we call the
        # program.
        client = find_client()
        command.setup_client(client, options, config)
        return command, args
    except KeyError as error:
        log_warning("WBFabric Module", "Error (%s). Command (%s %s) was not found." % (error, group_name, command_name))
        raise KeyError("Error (%s). Command (%s %s) was not found." % (error, group_name, command_name))

    return None
# ----------------------------------------------------------------------

def execute_command(group_name, command_name, options, args, config):
  command, args = create_command(group_name, command_name, options, args, config)
  
  return command.client.dispatch(command, *(command.append_options_to_args(args)))


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def testConnection(conn):

    error = fabric_unavailable_message

    if not error:
        try:
            config = create_config(conn)

            if config:
                status = execute_command('manage', 'ping', None, None, config)
        
                if not status:
                    error = "Unexpected error while connecting to the fabric node"
        
        except OperationCancelledError, e:
            error = "Operation Cancelled"
            log_warning("WBFabric Module", "User cancelled testing MySQL Fabric connection %s\n" % conn.name)
        except Exception, e:
            error = str(e)
            log_warning("WBFabric Module", "Error testing MySQL Fabric connection %s : %s\n" % (conn.name, traceback.format_exc()))

    return error


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def createConnections(conn):
  
    error = fabric_unavailable_message

    if not error:
        try:
            # Creates the configuration object from the connection
            # settings.
            config = create_config(conn)
        
            if config:

                # Pulls the HA groups
                group_status = execute_command('group', 'lookup_groups', None, None, config)

                # Variables for error handling
                fabric_group_count = 0
                filter_group_count = 0
                matched_groups = []
                server_count = 0

                # If all is OK continues pulling the servers for each group
                success = group_status[0]
                if success:
                    groups = group_status[2]

                    fabric_grounp_count = len(groups)

                    group_filter = conn.parameterValues["haGroupFilter"].strip()
                    group_list = []
                    if group_filter:
                        group_list = [group.strip() for group in group_filter.split(',')]
                        filter_group_count = len(group_list)

                    for group in groups:
                        include_group = not group_filter or group['group_id'] in group_filter

                        if include_group:
                            matched_groups.append(group['group_id'])
                            servers_status = execute_command('group', 'lookup_servers', None, [group['group_id']], config)
                            success = servers_status[0]

                            if success:
                                servers = servers_status[2]

                                # Creates a connection for each retrieved server.
                                for server in servers:
                                    address = server['address']
                                    host, port = address.split(':')
                            
                                    # If the managed servers are located on the fabric node
                                    # most probably they will use localhost or 127.0.0.1 as
                                    # address on the fabric configuration.

                                    # We need to replace that for the fabric node IP in order
                                    # to create the connections in WB
                                    if host in ['localhost', '127.0.0.1']:
                                        address = config.get('protocol.xmlrpc', 'address')
                                        host, _ = address.split(':')

                                    child_conn_name = '%s/%s/%s:%s' % (conn.name, group['group_id'], host, port)
                            
                                    server_user = conn.parameterValues["mysqlUserName"]
                                    managed_conn = grt.modules.Workbench.create_connection(host, server_user, '', 1, 0, int(port), child_conn_name)
                                    managed_conn.parameterValues["fabric_managed"] = True

                                    server_count += 1

                    if server_count:
                        grt.modules.Workbench.refreshHomeConnections()
                    else:
                        if fabric_grounp_count == 0:
                            error = "There are no High Availability Groups defined on the %s fabric node." % conn.name
                        elif not matched_groups:
                            error = "There are no High Availability Groups matching the configured group filter on %s." % conn.name
                        else:
                            error = "There are no Managed Servers defined for the included groups in %s: %s." % (conn.name, ','.join(matched_groups))

        except OperationCancelledError, e:
            error = "Operation Cancelled"
            log_warning("WBFabric Module", "User cancelled creating connectios to managed servers in the %s fabric node\n" % conn.name)
        except Exception, e:
            error = str(e)
            log_warning("WBFabric Module", "Error creating connectios to managed servers in the %s fabric node : %s\n" % (conn.name, traceback.format_exc()))
    
    return error


