/*
 * Copyright (c) 2012, 2021, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#ifdef _MSC_VER
#define HAVE_ROUND
#endif

#ifdef __APPLE__
// All the functions in sql.h are deprecated, but we have no replacement atm.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "python_copy_data_source.h" // python stuff need to be 1st #include
#include "copytable.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include "base/log.h"
#include "base/sqlstring.h"

#undef tolower
#undef toupper

#include "base/file_utilities.h"
#include "base/string_utilities.h"

#include "workbench/wb_version.h"

#include "SSHTunnelManager.h"

void *get_mainwindow_impl() {
    return 0;
}

class input_error : public std::runtime_error {
public:
  input_error(const std::string &what) : std::runtime_error(what) {}
};

DEFAULT_LOG_DOMAIN("copytable");

static void count_rows(std::unique_ptr<CopyDataSource> &source,
                       const std::string &source_schema,
                       const std::string &source_table,
                       const std::vector<std::string> &pk_columns,
                       const CopySpec &spec,
                       const std::vector<std::string> &last_pkeys) {
  unsigned long long total = source->count_rows(source_schema, source_table,
                                                pk_columns, spec, last_pkeys);

  printf("ROW_COUNT:%s:%s: %llu\n", source_schema.c_str(), source_table.c_str(),
         total);
  fflush(stdout);
}

//-----------------------

static bool set_log_level(const std::string &value) {
  std::string level = base::tolower(value);
  bool ret = base::Logger::active_level(level);
  if (ret) // TODO: if the logger is set to error or warning the following log
           // call won't do anything.
    logInfo("Logger set to level '%s'. '%s'\n", level.c_str(),
            base::Logger::get_state().c_str());

  return ret;
}

static bool check_arg_with_value(char **argv, int &argi, const char *arg,
                                 char *&value, bool arg_required) {
  char *a = argv[argi];

  if (strcmp(a, arg) == 0) {
    // value must be in next arg
    if (argv[argi + 1] != NULL) {
      ++argi;
      value = argv[argi];
    } else {
      value = NULL;
      if (arg_required) {
        fprintf(stderr, "Missing argument for option %s\n", argv[argi]);
        return false;
      }
    }
    return true;
  } else if (strncmp(a, arg, strlen(arg)) == 0 && a[strlen(arg)] == '=') {
    // value must be after =
    value = a + strlen(arg) + 1;
    return true;
  }
  return false;
}

static bool parse_mysql_connstring(const std::string &connstring,
                                   std::string &user, std::string &password,
                                   std::string &host, int &port,
                                   std::string &sock) {
  // Format is user[:pass]@host:port or user[:pass]@::socket,
  // like what cmdline utilities use.
  std::string::size_type p = connstring.rfind('@');
  if (p == std::string::npos)
    return false;

  std::string user_part = connstring.substr(0, p);
  std::string server_part = connstring.substr(p + 1);

  if ((p = user_part.find(':')) != std::string::npos) {
    user = user_part.substr(0, p);
    password = user_part.substr(p + 1);
  } else
    user = user_part;

  p = server_part.find(':');
  if (p != std::string::npos) {
    host = server_part.substr(0, p);
    server_part = server_part.substr(p + 1);
    p = server_part.find(':');
    if (p != std::string::npos)
      sock = server_part.substr(p + 1);
    else if (!sscanf(server_part.substr(0, p).c_str(), "%i", &port))
      return false;
  } else
    host = server_part;
  return true;
}

static void show_help() {
  printf("copytable --*-source=<source db> --target=<target db> <options> "
         "<table spec> [<table spec> ...]\n");
  printf("--odbc-source=<odbc connstring>\n");
  printf("--pythondbapi-source=<python connstring>\n");
  printf("--mysql-source=<mysql connstring>\n");
  printf("--source-password=<password>\n");
  printf("--source-ssh-port=<ssh port>\n");
  printf("--source-ssh-host=<ssh host>\n");
  printf("--source-ssh-user=<ssh user>\n");
  printf("--source-ssh-password=<ssh password>\n");
  printf("--source-ssh-key=<path to ssh key>\n");
  printf("--target=<mysql connstring>\n");
  printf("--target-password=<password>\n");
  printf("--target-ssh-port=<ssh port>\n");
  printf("--target-ssh-host=<ssh host>\n");
  printf("--target-ssh-user=<ssh user>\n");
  printf("--target-ssh-password=<ssh password>\n");
  printf("--target-ssh-key=<path to ssh key>\n");
  printf("--ssh-known-hosts-file=<path to ssh known hosts file>\n");
  printf("--ssh-config-file=<path to ssh config file>\n");
  printf("--force-utf8-for-source\n");
  printf("--truncate-target\n");
  printf("--progress\n");
  printf("--count-only\n");
  printf("--jobs-from-stdin\n");
  printf("--abort-on-oversized-blobs\n");
  printf("--max-count=<max rows count>\n");
  printf("--resume\n");
  printf("Table Specification from file:\n");
  printf("--table-file=<filename>\n");
  printf("<source schema><TAB><source table><TAB><target schema><TAB><target "
         "table><TAB><source pk columns><TAB><target pk "
         "columns><TAB>*|<select expression>\n");
  printf("Table Specification from command line:\n");
  printf("--table <source schema> <source table> <target schema> <target "
         "table> <source pk columns> <target pk columns> "
         "*|<select expression>\n");
  printf("--table-where <source schema> <source table> <target schema> <target "
         "table> <source pk columns> <target pk "
         "columns> *|<select expression> <where expression>\n");
  printf("\n");
  printf("--log-file=<file_path>\n");
  printf("--log-level=<level>\n");
  printf("--thread-count=<count>\n");
  printf("--bulk-insert-batch-size=<size>\n");
  printf("--disable-triggers-on=<schema>\n");
  printf("--reenable-triggers-on=<schema>\n");
  printf("--dont-disable-triggers");
  printf("--version\n");
  printf("--help\n");
}

/*
* read_tasks_from_file : reads the table information from a text file.
* Parameters:
* - file_name : the file containing the table definitions
* - count_only : indicates if the file contains information to count the records
*                from the source DB or to actually trasmit the data
* - tasks : output parameter that will contain a task for each table definition
loaded
*           from the file
  - resume : indicates if the file contains information to resume copying data
from
             last PK
* - max_count : limit copied rows count to max_count
*
* Remarks : Each table is defined in a single line with the next format for
count_only = true and resume = false
*           <src_schema>\t<src_table>\n
*
*           and in the next format for a count_only = false or resume = true
*
<src_schema>\t<src_table>\t<tgt_schema>\t<tgt_table>\t<source_pk_columns>\t<target_pk_columns>\t<select_expression>
*/
bool read_tasks_from_file(const std::string file_name, bool count_only,
                          TaskQueue &tasks,
                          std::set<std::string> &trigger_schemas, bool resume,
                          long long int max_count) {
  std::ifstream ifs(file_name.data(), std::ifstream::in);
  unsigned int field_count = count_only && !resume ? 2 : 7;
  bool error = false;

  printf("Loading table information from file %s\n", file_name.data());

  while (!error && ifs.good()) {
    TableParam param;
    std::string line;
    getline(ifs, line);

    if (line.length()) {
      logInfo("--table %s\n", line.data());

      std::vector<std::string> fields = base::split(line, "\t", field_count);

      if (fields.size() == field_count) {
        param.source_schema = fields[0];
        param.source_table = fields[1];

        if (!(count_only && !resume)) {
          param.target_schema = fields[2];
          param.target_table = fields[3];
          if (std::strcmp(fields[4].c_str(), "-") != 0)
            param.source_pk_columns = base::split(fields[4], ",", -1);
          if (std::strcmp(fields[5].c_str(), "-") != 0)
            param.target_pk_columns = base::split(fields[5], ",", -1);
          param.select_expression = fields[6];

          trigger_schemas.insert(param.target_schema);
        }

        param.copy_spec.resume = resume;
        param.copy_spec.max_count = max_count;
        param.copy_spec.type = CopyAll;
        tasks.add_task(param);
      } else
        error = true;
    }
  }

  ifs.close();

  return !error;
}

uint16_t createTunnel(ssh::SSHConnectionConfig &config,
                      const ssh::SSHConnectionCredentials &credentials,
                      ssh::SSHTunnelManager *manager) {
  uint16_t tunnel_port = manager->lookupTunnel(config);
  if (tunnel_port > 0) {
    logInfo("Existing SSH tunnel found, connecting...\n");
    return tunnel_port;
  }

  auto session = ssh::SSHSession::createSession();
  std::tuple<ssh::SSHReturnType, base::any> retVal;
  bool connected = false;

  while (!connected) {
    try {
      retVal = session->connect(config, credentials);
    } catch (std::exception &e) {
      throw std::runtime_error(e.what());
    }

    std::string errorMsg = "";
    switch (std::get<0>(retVal)) {
      case ssh::SSHReturnType::CONNECTED: {
        connected = true;
        break;
      }
      case ssh::SSHReturnType::FINGERPRINT_CHANGED:
      case ssh::SSHReturnType::FINGERPRINT_MISMATCH: {
        std::string fingerprint = std::get<1>(retVal);
        errorMsg = "WARNING: Server public key has changed. It means either "
                  "you're under attack or the administrator has changed the "
                  "key. New public fingerprint is: " +
                  fingerprint;
        throw std::runtime_error(errorMsg);
        break;
      }
      case ssh::SSHReturnType::FINGERPRINT_UNKNOWN:
      case ssh::SSHReturnType::FINGERPRINT_UNKNOWN_AUTH_FILE_MISSING: {
        std::string fingerprint = std::get<1>(retVal);
        std::string msg = "The authenticity of host '" + config.remoteSSHhost +
                          "' can't be established.\n Server key fingerprint is " +
                          fingerprint +
                          "\nAre you sure you want to continue connecting? [y/N]";
        std::cout << msg;
        char retChar = 'N';
        std::cin >> retChar;
        if (toupper(retChar) == 'Y') {
          config.fingerprint = fingerprint;
          session->disconnect();
        } else {
          errorMsg = "The authenticity of host '" + config.remoteSSHhost +
                    "' can't be established.";
          throw std::runtime_error(errorMsg);
        }
        break;
      }
      default: {
        std::string errorMsg = std::get<1>(retVal);
        throw std::runtime_error(errorMsg);
      }
    }
  }

  try {
    retVal = manager->createTunnel(session);
  } catch (ssh::SSHTunnelException &se) {
    std::string errMsg =
        std::string("Unable to create tunnel: ").append(se.what());
    throw std::runtime_error(errMsg);
  }
  if (std::get<0>(retVal) != ssh::SSHReturnType::CONNECTED) {
    std::string errorMsg = std::get<1>(retVal);
    throw std::runtime_error(errorMsg);
  }

  return (uint16_t)std::get<1>(retVal);
}

int main(int argc, char **argv) {
  std::string app_name = base::basename(argv[0]);

  TaskQueue tables;

  std::string source_password;
  std::string source_connstring;
  bool source_use_cleartext_plugin = false;
  bool source_is_utf8 = false;
  std::string source_charset;
  SourceType source_type = ST_MYSQL;

  std::string target_connstring;
  std::string target_password;
  bool target_use_cleartext_plugin = false;
  std::string log_level;
  std::string log_file;

  bool passwords_from_stdin = false;
  bool count_only = false;
  bool check_types_only = false;
  bool truncate_target = false;
  bool show_progress = false;
  bool abort_on_oversized_blobs = false;
  bool disable_triggers = false;
  bool reenable_triggers = false;
  bool disable_triggers_on_copy = true;
  bool resume = false;
  int thread_count = 1;
  long long bulk_insert_batch = 100;
  long long max_count = 0;

  std::string table_file;

  std::set<std::string> trigger_schemas;
  std::string source_rdbms_type = "unknown";
  unsigned int target_connection_timeout = 60;
  unsigned int source_connection_timeout = 60;

  ssh::SSHConnectionConfig sourceConfig;
  sourceConfig.localhost = "127.0.0.1";
  sourceConfig.remotehost = "127.0.0.1";
  sourceConfig.remoteport = 0;
  sourceConfig.remoteSSHhost = "";
  sourceConfig.remoteSSHport = 22;
  sourceConfig.connectTimeout = 10;
  sourceConfig.bufferSize = 10240;
  sourceConfig.connectTimeout = 10;
  sourceConfig.readWriteTimeout = 5;
  sourceConfig.commandTimeout = 1;
  sourceConfig.commandRetryCount = 3;
  sourceConfig.compressionLevel = 0;

  ssh::SSHConnectionCredentials sourceCredentials;
  sourceCredentials.username = "";
  sourceCredentials.password = "";
  sourceCredentials.auth = ssh::SSHAuthtype::PASSWORD;

  ssh::SSHConnectionConfig targetConfig;
  targetConfig.localhost = "127.0.0.1";
  targetConfig.remotehost = "127.0.0.1";
  targetConfig.remoteport = 0;
  targetConfig.remoteSSHhost = "";
  targetConfig.remoteSSHport = 22;
  targetConfig.connectTimeout = 10;
  targetConfig.bufferSize = 10240;
  targetConfig.connectTimeout = 10;
  targetConfig.readWriteTimeout = 5;
  targetConfig.commandTimeout = 1;
  targetConfig.commandRetryCount = 3;
  targetConfig.compressionLevel = 0;

  ssh::SSHConnectionCredentials targetCredentials;
  targetCredentials.username = "";
  targetCredentials.password = "";
  targetCredentials.auth = ssh::SSHAuthtype::PASSWORD;

  bool log_level_set = false;
  int i = 1;
  while (i < argc) {
    char *argval = NULL;

    if (check_arg_with_value(argv, i, "--log-level", argval, true))
      log_level = argval;
    else if (check_arg_with_value(argv, i, "--log-file", argval, true))
      log_file = argval;
    else if (check_arg_with_value(argv, i, "--odbc-source", argval, true)) {
      source_type = ST_ODBC;
      source_connstring = base::trim(argval, "\"");
    } else if (check_arg_with_value(argv, i, "--mysql-source", argval, true)) {
      source_type = ST_MYSQL;
      source_connstring = base::trim(argval, "\"");
    } else if (check_arg_with_value(argv, i, "--pythondbapi-source", argval, true)) {
      source_type = ST_PYTHON;
      source_connstring = base::trim(argval, "\"");
    } else if (check_arg_with_value(argv, i, "--source-password", argval, true))
      source_password = argval;
    else if (check_arg_with_value(argv, i, "--target-password", argval, true))
      target_password = argval;
    else if (strcmp(argv[i], "--force-utf8-for-source") == 0)
      source_is_utf8 = true;
    else if (check_arg_with_value(argv, i, "--source-charset", argval, true))
      source_charset = argval;
    else if (strcmp(argv[i], "--progress") == 0)
      show_progress = true;
    else if (strcmp(argv[i], "--truncate-target") == 0)
      truncate_target = true;
    else if (strcmp(argv[i], "--count-only") == 0) {
      // Count only will be allowed only if one of the trigger
      // operations has not been indicated first
      if (!disable_triggers && !reenable_triggers)
        count_only = true;
    } else if (strcmp(argv[i], "--check-types-only") == 0)
      check_types_only = true;
    else if (strcmp(argv[i], "--passwords-from-stdin") == 0)
      passwords_from_stdin = true;
    else if (strcmp(argv[i], "--abort-on-oversized-blobs") == 0)
      abort_on_oversized_blobs = true;
    else if (strcmp(argv[i], "--dont-disable-triggers") == 0)
      disable_triggers_on_copy = false;
    else if (strcmp(argv[i], "--resume") == 0)
      resume = true;
    else if (check_arg_with_value(argv, i, "--disable-triggers-on", argval, true)) {
      // disabling/enabling triggers are standalone operations and mutually
      // exclusive
      // so here it ensures a request for trigger enabling was not found first
      if (!reenable_triggers && !count_only) {
        disable_triggers = true;
        trigger_schemas.insert(argval);
      }
    } else if (check_arg_with_value(argv, i, "--reenable-triggers-on", argval, true)) {
      // disabling/enabling triggers are standalone operations and mutually
      // exclusive
      // so here it ensures a request for trigger enabling was not found first
      if (!disable_triggers && !count_only) {
        reenable_triggers = true;
        trigger_schemas.insert(argval);
      }
    } else if (check_arg_with_value(argv, i, "--thread-count", argval, true)) {
      thread_count = base::atoi<int>(argval, 0);
      if (thread_count < 1)
        thread_count = 1;
    } else if (check_arg_with_value(argv, i, "--bulk-insert-batch-size", argval, true)) {
      bulk_insert_batch = base::atoi<int>(argval, 0);
      if (bulk_insert_batch < 1)
        bulk_insert_batch = 100;
    } else if (check_arg_with_value(argv, i, "--source-ssh-port", argval, true))
      sourceConfig.remoteSSHport = base::atoi<int>(argval, 0);
    else if (check_arg_with_value(argv, i, "--source-ssh-host", argval, true))
      sourceConfig.remoteSSHhost = argval;
    else if (check_arg_with_value(argv, i, "--source-ssh-user", argval, true))
      sourceCredentials.username = argval;
    else if (check_arg_with_value(argv, i, "--source-ssh-password", argval, true)) {
      sourceCredentials.password = argval;
      sourceCredentials.auth = ssh::SSHAuthtype::PASSWORD;
    } else if (check_arg_with_value(argv, i, "--source-ssh-key", argval, true)) {
      sourceCredentials.keyfile = argval;
      sourceCredentials.auth = ssh::SSHAuthtype::KEYFILE;
    } else if (check_arg_with_value(argv, i, "--target-ssh-port", argval, true))
      targetConfig.remoteSSHport = base::atoi<int>(argval, 0);
    else if (check_arg_with_value(argv, i, "--target-ssh-host", argval, true))
      targetConfig.remoteSSHhost = argval;
    else if (check_arg_with_value(argv, i, "--target-ssh-user", argval, true))
      targetCredentials.username = argval;
    else if (check_arg_with_value(argv, i, "--target-ssh-password", argval, true)) {
      targetCredentials.password = argval;
      targetCredentials.auth = ssh::SSHAuthtype::PASSWORD;
    } else if (check_arg_with_value(argv, i, "--target-ssh-key", argval, true)) {
      targetCredentials.keyfile = argval;
      targetCredentials.auth = ssh::SSHAuthtype::KEYFILE;
    } else if (check_arg_with_value(argv, i, "--ssh-known-hosts-file", argval, true)) {
      sourceConfig.knownHostsFile = targetConfig.knownHostsFile = argval;
    } else if (check_arg_with_value(argv, i, "--ssh-config-file", argval, true)) {
      sourceConfig.configFile = targetConfig.configFile = argval;
    } else if (strcmp(argv[i], "--version") == 0) {
      const char *type = APP_EDITION_NAME;
      if (strcmp(APP_EDITION_NAME, "Community") ==
          (0)) // Extra parens to silence warning.
        type = "CE";

      printf("%s %s (%s) %i.%i.%i %s build %i\n",
             base::basename(argv[0]).c_str(), type, APP_LICENSE_TYPE,
             APP_MAJOR_NUMBER, APP_MINOR_NUMBER, APP_RELEASE_NUMBER,
             APP_RELEASE_TYPE, APP_BUILD_NUMBER);
      exit(0);
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      show_help();
      exit(0);
    } else if (check_arg_with_value(argv, i, "--target", argval, true)) {
      target_connstring = base::trim(argval, "\"");
    } else if (check_arg_with_value(argv, i, "--table-file", argval, true))
      table_file = argval;
    else if (strcmp(argv[i], "--table") == 0) {
      TableParam param;

      if ((!count_only && i + 7 >= argc) || (count_only && i + 2 >= argc)) {
        fprintf(stderr, "%s: Missing value for table copy specification\n",
                argv[0]);
        exit(1);
      }

      param.source_schema = argv[++i];
      param.source_table = argv[++i];
      if (!(count_only && !resume)) {
        param.target_schema = argv[++i];
        param.target_table = argv[++i];
        if (std::strcmp(argv[++i], "-") != 0)
          param.source_pk_columns = base::split(argv[i], ",", -1);
        if (std::strcmp(argv[++i], "-") != 0)
          param.target_pk_columns = base::split(argv[i], ",", -1);
        param.select_expression = argv[++i];

        trigger_schemas.insert(param.target_schema);
      }

      param.copy_spec.resume = resume;
      param.copy_spec.max_count = max_count;

      param.copy_spec.type = CopyAll;

      tables.add_task(param);
    } else if (strcmp(argv[i], "--table-range") == 0) {
      TableParam param;

      if ((!count_only && i + 10 >= argc) || (count_only && i + 5 >= argc)) {
        fprintf(stderr, "%s: Missing value for table copy specification\n",
                argv[0]);
        exit(1);
      }
      param.source_schema = argv[++i];
      param.source_table = argv[++i];
      if (!(count_only && !resume)) {
        param.target_schema = argv[++i];
        param.target_table = argv[++i];
        if (std::strcmp(argv[++i], "-") != 0)
          param.source_pk_columns = base::split(argv[i], ",", -1);
        if (std::strcmp(argv[++i], "-") != 0)
          param.target_pk_columns = base::split(argv[i], ",", -1);
        param.select_expression = argv[++i];

        trigger_schemas.insert(param.target_schema);
      }
      param.copy_spec.range_key = argv[++i];
      param.copy_spec.range_start = base::atoi<long long>(argv[++i], 0ll);
      param.copy_spec.range_end = base::atoi<long long>(argv[++i], 0ll);
      param.copy_spec.type = CopyRange;

      tables.add_task(param);
    } else if (strcmp(argv[i], "--table-row-count") == 0) {
      TableParam param;

      if ((!count_only && i + 8 >= argc) || (count_only && i + 3 >= argc)) {
        fprintf(stderr, "%s: Missing value for table copy specification\n",
                argv[0]);
        exit(1);
      }
      param.source_schema = argv[++i];
      param.source_table = argv[++i];
      if (!(count_only && !resume)) {
        param.target_schema = argv[++i];
        param.target_table = argv[++i];
        if (std::strcmp(argv[++i], "-") != 0)
          param.source_pk_columns = base::split(argv[i], ",", -1);
        if (std::strcmp(argv[++i], "-") != 0)
          param.target_pk_columns = base::split(argv[i], ",", -1);
        param.select_expression = argv[++i];
      }
      param.copy_spec.row_count = base::atoi<long long>(argv[++i], 0ll);
      param.copy_spec.resume = resume;
      param.copy_spec.type = CopyCount;

      tables.add_task(param);
    } else if (check_arg_with_value(argv, i, "--source-rdbms-type", argval,
                                    false))
      source_rdbms_type = argval;
    else if (strcmp(argv[i], "--table-where") == 0) {
      TableParam param;

      if ((!count_only && i + 8 >= argc) || (count_only && i + 4 >= argc)) {
        fprintf(stderr, "%s: Missing value for table copy specification\n", argv[0]);
        exit(1);
      }
      param.source_schema = argv[++i];
      param.source_table = argv[++i];
      if (!(count_only && !resume)) {
        param.target_schema = argv[++i];
        param.target_table = argv[++i];
        if (std::strcmp(argv[++i], "-") != 0)
          param.source_pk_columns = base::split(argv[i], ",", -1);
        if (std::strcmp(argv[++i], "-") != 0)
          param.target_pk_columns = base::split(argv[i], ",", -1);
        param.select_expression = argv[++i];
        param.copy_spec.where_expression = argv[++i];

        trigger_schemas.insert(param.target_schema);
      } else {
        param.select_expression = argv[++i];
        param.copy_spec.where_expression = argv[++i];
      }
      param.copy_spec.type = CopyWhere;

      tables.add_task(param);
    } else if (check_arg_with_value(argv, i, "--max-count", argval, true)) {
      max_count = base::atoi<int>(argval, 0);
    } else if (strcmp(argv[i], "--source-use-cleartext") == 0)
      source_use_cleartext_plugin = true;
    else if (strcmp(argv[i], "--target-use-cleartext") == 0)
      target_use_cleartext_plugin = true;
    else if (check_arg_with_value(argv, i, "--target-timeout", argval, true))
      target_connection_timeout = base::atoi<int>(argval, 0);
    else if (check_arg_with_value(argv, i, "--source-timeout", argval, true))
      source_connection_timeout = base::atoi<int>(argval, 0);
    else {
      fprintf(stderr, "%s: Invalid option %s\n", argv[0], argv[i]);
      exit(1);
    }

    i++;
  }

  // Creates the log to the target file if any, if not
  // uses std_error
  base::Logger logger(true, log_file);

  if (!log_level.empty()) {
    if (!set_log_level(log_level)) {
      fprintf(stderr, "%s: invalid argument '%s' for option %s\n", argv[0], log_level.data(), "--log-level");
      exit(1);
    } else
      log_level_set = true;
  }

  // Set the log level from environment var WB_LOG_LEVEL if specified or set a
  // default log level.
  if (!log_level_set) {
    const char *log_setting = getenv("WB_LOG_LEVEL");
    if (log_setting == NULL)
      log_setting = "info";
    else
      log_level_set = true;

    std::string level = base::tolower(log_setting);
    base::Logger::active_level(level);
  }

  // If needed, reads the tasks from the table definition file
  if (!table_file.empty()) {
    if (!read_tasks_from_file(table_file, count_only, tables, trigger_schemas, resume, max_count)) {
      fprintf(stderr, "Invalid table definitions format in file: %s\n", table_file.data());
      exit(1);
    }
  }

  // Not having the source connection data is an error unless
  // the standalone operations to disable or reenable triggers
  // are called
  if (source_connstring.empty() && !reenable_triggers && !disable_triggers) {
    fprintf(stderr, "Missing source DB server\n");
    exit(1);
  }

  if (target_connstring.empty() && !(count_only && !resume)) {
    fprintf(stderr, "Missing target DB server\n");
    exit(1);
  }

  // Table definitions will be required only if the standalone operations to
  // Reenable or disable triggers are not called
  if (tables.empty() && !reenable_triggers && !disable_triggers) {
    logWarning("Missing table list specification\n");
    exit(0);
  }

  std::string source_host;
  std::string source_user;
  int source_port = -1;
  std::string source_socket;

  // Source connection is parsed only when NOT executing the
  // Standalone operatios on triggers
  if (source_type == ST_MYSQL && !reenable_triggers && !disable_triggers) {
    if (!parse_mysql_connstring(source_connstring, source_user, source_password, 
                                source_host, source_port, source_socket)) {
      fprintf(stderr, "Invalid MySQL connection string %s for source database. "
                      "Must be in format user[:pass]@host:port or "
                      "user[:pass]@::socket\n",
              source_connstring.c_str());
      exit(1);
    } else {
      sourceConfig.remotehost = source_host;
      sourceConfig.remoteport = source_port;
    }
  }

  std::string target_host;
  std::string target_user;
  int target_port = -1;
  std::string target_socket;
  if (!(count_only && !resume) &&
      !parse_mysql_connstring(target_connstring, target_user, target_password,
                              target_host, target_port, target_socket)) {
    fprintf(stderr, "Invalid MySQL connection string %s for target database. "
                    "Must be in format user[:pass]@host:port or "
                    "user[:pass]@::socket\n",
            target_connstring.c_str());
    exit(1);
  } else {
    targetConfig.remotehost = target_host;
    targetConfig.remoteport = target_port;
  }

  if (passwords_from_stdin) {
    char password[200];
    if (!fgets(password, sizeof(password), stdin)) {
      logError("Error reading passwords from stdin\n");
      exit(1);
    }

    if ((count_only && !resume) || reenable_triggers || disable_triggers) {
      char *ptr = strtok(password, "\t\r\n");
      if (ptr) {
        if (count_only)
          source_password = ptr;
        else
          target_password = ptr;
      }
    } else {
      char *ptr = strtok(password, "\r\n");
      if (ptr) {
        ptr = strchr(password, '\t');
        if (ptr) {
          source_password = std::string(password, ptr - password);
          target_password = ptr + 1;
        } else
          source_password = password;
      }
    }
  }

  static SQLHENV odbc_env;
  ssh::SSHTunnelManager *manager = nullptr;

  if (!sourceConfig.remoteSSHhost.empty() || !targetConfig.remoteSSHhost.empty()) {
    manager = new ssh::SSHTunnelManager();
    manager->start();
    if (!sourceConfig.remoteSSHhost.empty()) {
      source_port = createTunnel(sourceConfig, sourceCredentials, manager);
      source_host = "127.0.0.1";
    }
    if (!targetConfig.remoteSSHhost.empty()) {
      target_port = createTunnel(targetConfig, targetCredentials, manager);
      target_host = "127.0.0.1";
    }
  }

  PyThreadState *state = NULL;
  if (source_type == ST_PYTHON) {
    Py_Initialize();
    //PyEval_InitThreads();
    state = PyEval_SaveThread();
  }
  try {
    if (count_only) {
      std::unique_ptr<CopyDataSource> psource;

      if (source_type == ST_ODBC) {
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbc_env);
        SQLSetEnvAttr(odbc_env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);

        psource.reset(new ODBCCopyDataSource(odbc_env, source_connstring,
                                             source_password, source_is_utf8,
                                             source_rdbms_type));
      } else if (source_type == ST_MYSQL)
        psource.reset(new MySQLCopyDataSource(
            source_host, source_port, source_user, source_password,
            source_socket, source_use_cleartext_plugin,
            source_connection_timeout));
      else
        psource.reset(
            new PythonCopyDataSource(source_connstring, source_password));

      std::unique_ptr<MySQLCopyDataTarget> ptarget;
      TableParam task;
      while (tables.get_task(task)) {
        std::vector<std::string> last_pkeys;
        if (task.copy_spec.resume) {
          if (!ptarget.get())
            ptarget.reset(new MySQLCopyDataTarget(
                target_host, target_port, target_user, target_password,
                target_socket, target_use_cleartext_plugin, app_name,
                source_charset, source_rdbms_type, target_connection_timeout));
          last_pkeys = ptarget->get_last_pkeys(task.target_pk_columns, task.target_schema, task.target_table);
        }
        count_rows(psource, task.source_schema, task.source_table, task.source_pk_columns, task.copy_spec, last_pkeys);
      }
    } else if (reenable_triggers || disable_triggers) {
      std::unique_ptr<MySQLCopyDataTarget> ptarget;
      ptarget.reset(new MySQLCopyDataTarget(
          target_host, target_port, target_user, target_password, target_socket,
          target_use_cleartext_plugin, app_name, source_charset,
          source_rdbms_type, target_connection_timeout));

      if (disable_triggers)
        ptarget->backup_triggers(trigger_schemas);
      else
        ptarget->restore_triggers(trigger_schemas);
    } else {
      std::vector<CopyDataTask *> threads;

      std::unique_ptr<MySQLCopyDataTarget> ptarget_conn;
      MySQLCopyDataTarget *ptarget = NULL;
      CopyDataSource *psource = NULL;

      if (disable_triggers_on_copy) {
        ptarget_conn.reset(new MySQLCopyDataTarget(
            target_host, target_port, target_user, target_password,
            target_socket, target_use_cleartext_plugin, app_name,
            source_charset, source_rdbms_type, target_connection_timeout));
        ptarget_conn->backup_triggers(trigger_schemas);
      }

      for (int index = 0; index < thread_count; index++) {
        if (source_type == ST_ODBC) {
          SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &odbc_env);
          SQLSetEnvAttr(odbc_env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);

          psource = new ODBCCopyDataSource(odbc_env, source_connstring,
                                           source_password, source_is_utf8,
                                           source_rdbms_type);
        } else if (source_type == ST_MYSQL)
          psource = new MySQLCopyDataSource(
              source_host, source_port, source_user, source_password,
              source_socket, source_use_cleartext_plugin,
              source_connection_timeout);
        else
          psource = new PythonCopyDataSource(source_connstring, source_password);

        ptarget = new MySQLCopyDataTarget(
            target_host, target_port, target_user, target_password,
            target_socket, target_use_cleartext_plugin, app_name,
            source_charset, source_rdbms_type, target_connection_timeout);

        psource->set_max_blob_chunk_size(ptarget->get_max_allowed_packet());
        psource->set_max_parameter_size((unsigned long)ptarget->get_max_long_data_size());
        psource->set_abort_on_oversized_blobs(abort_on_oversized_blobs);
        ptarget->set_truncate(truncate_target);
        if (max_count > 0)
          bulk_insert_batch = max_count;
        ptarget->set_bulk_insert_batch_size((int)bulk_insert_batch);

        if (check_types_only) {
          // XXXX
          delete psource;
        } else {
          threads.push_back(new CopyDataTask(base::strfmt("Task %d", index + 1),
                                             psource, ptarget, &tables,
                                             show_progress));
        }
      }

      // Waits for all the threads to complete
      for (size_t index = 0; index < threads.size(); index++)
        threads[index]->wait();

      // Finally destroys the threads and connections
      for (size_t index = 0; index < threads.size(); index++)
        delete threads[index];

      // Finally restores the triggers
      if (disable_triggers_on_copy)
        ptarget_conn->restore_triggers(trigger_schemas);
    }
  } catch (std::exception &e) {
    logError("Exception: %s\n", e.what());
    if (source_type == ST_PYTHON) {
      PyEval_RestoreThread(state);
      Py_Finalize();
    }
    exit(1);
  }

  if (source_type == ST_PYTHON) {
    PyEval_RestoreThread(state);
    Py_Finalize();
  }

  if (manager) {
    manager->setStop();
    manager->pokeWakeupSocket();
    if (manager->isRunning())
      manager->join();
    delete manager;
    manager = nullptr;
  }

  printf("FINISHED\n");
  fflush(stdout);

  return 0;
}
