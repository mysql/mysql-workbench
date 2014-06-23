# -*- coding: utf-8 -*-
# MySQL Workbench module
# <description>
# Written in MySQL Workbench 6.2.0

from wb import *
import grt
from grt import log_warning
from mysql.fabric.config import Config
from mysql.fabric.credentials import check_credentials
import mforms

from mysql.fabric.command import get_groups, get_command, get_commands
from mysql.fabric.services import find_client, find_commands
from mysql.fabric.options import OptionParser


ModuleInfo = DefineModule(name= "WBFabric", author= "Oracle Corp.", version="1.0")



def create_config(conn):
  # Retrieves the fabric node connection data
  host = conn.parameterValues["hostName"]
  port = conn.parameterValues["port"]
  user = conn.parameterValues["userName"]
  
  # Retrieves the backing store connection data
  bs_host = conn.parameterValues["bsHostName"]
  bs_port = conn.parameterValues["bsPort"]
  bs_user = conn.parameterValues["bsUserName"]
  bs_database = conn.parameterValues["bsDatabase"]
  
  accepted, password = mforms.Utilities.find_or_ask_for_password("Fabric Node Connection", '%s@%s' % (host, port), user, False)
  if accepted:
    accepted, bs_password = mforms.Utilities.find_or_ask_for_password("Backing Store Connection", '%s@%s' % (bs_host, bs_port), bs_user, False)
    
    if accepted:
      config = Config(None, {'protocol.xmlrpc':{'address':'%s:%s' % (host, port), 'user':user, 'password':password, 'realm':'MySQL Fabric'}, 'storage':{'address':'%s:%s' % (bs_host, bs_port), 'user':bs_user,'password':bs_password,'database':bs_database}})
  
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
        log_warning("Error (%s). Command (%s %s) was not found." % (error, group_name, command_name))
        raise KeyError("Error (%s). Command (%s %s) was not found." % (error, group_name, command_name))

    return None
# ----------------------------------------------------------------------

def execute_command(group_name, command_name, options, args, config):
  command, args = create_command(group_name, command_name, options, args, config)
  
  return command.client.dispatch(command, *(command.append_options_to_args(args)))


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def test_connection(conn):
  
  config = create_config(conn)

  error = ""
  if config:
      try:
          check_credentials('user', '', config, "xmlrpc")
          
          status = execute_command('manage', 'ping', None, None, config)
      
          if not status:
              error = "Unexpected error while connecting to the fabric node"
      
      except Exception, e:
          error = str(e)

  return error


@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def create_connections(conn):
  
    ret_val = ""
    
    try:


        # Creates the configuration object
        config = create_config(conn)
        
        if config:
            group_status = execute_command('group', 'lookup_groups', None, None, config)

            success = group_status[0]
            
            if success:
                groups = group_status[2]

                for group in groups:
                    servers_status = execute_command('group', 'lookup_servers', None, [group['group_id']], config)
                    success = servers_status[0]

                    if success:
                        servers = servers_status[2]

                        for server in servers:
                            address = server['address']
                            host, port = address.split(':')
                            
                            if host in ['localhost', '127.0.0.1']:
                                address = config.get('protocol.xmlrpc', 'address')
                                host, _ = address.split(':')

                            db_user = config.get('storage', 'user')
                            child_conn_name = '%s/%s/%s:%s' % (conn.name, group['group_id'], host, port)
                            
                            grt.modules.Workbench.create_connection(host, db_user, '', 1, 0, int(port), child_conn_name)

                grt.modules.Workbench.refreshHomeConnections()

    except Exception, e:
      ret_val = str(e)
    
    return ret_val


