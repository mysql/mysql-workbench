/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <glib.h>
#include <string.h>
#include <pcre.h>
#include <algorithm>

#include "base/log.h"

#include "grtpp_helper.h"
#include "interfaces/interfaces.h"

using namespace grt;

//----------------------------------------------------------------------------------------------------------------------

void *get_mainwindow_impl() {
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

void register_all_interfaces() {
  register_interfaces();
}

//----------------------------------------------------------------------------------------------------------------------

void do_generate_interface_classes(const char *outfile, const std::vector<std::string> &interfaces) {
  register_all_interfaces();
  std::vector<grt::Module *> wanted;

  printf("Generating %s\n", outfile);

  for (std::map<std::string, grt::Interface *>::const_iterator intf = grt::GRT::get()->get_interfaces().begin();
       intf != grt::GRT::get()->get_interfaces().end(); ++intf) {
    if (interfaces.empty() ||
        std::find(interfaces.begin(), interfaces.end(), intf->second->name()) != interfaces.end()) {
      wanted.push_back(intf->second);
    }
  }

  if (!wanted.empty()) {
    if (!interfaces.empty() && interfaces.size() != wanted.size())
      fprintf(stderr, "WARNING: Some of the specified interfaces were not found\n");

    grt::helper::generate_module_wrappers(outfile, wanted);
  } else
    fprintf(stderr, "No interfaces to be wrapped.\n");
}

//----------------------------------------------------------------------------------------------------------------------

void generate_interface_classes(const char *header, const char *outfile) {
  std::vector<std::string> interfaces;
  char line[1024];
  FILE *f = fopen(header, "rb");
  if (!f) {
    fprintf(stderr, "ERROR: could not open header file '%s'\n", header);
    exit(1);
  }

  const char *errs;
  int erro;
  pcre *pat = pcre_compile("^\\s*DECLARE_REGISTER_INTERFACE\\(\\s*(\\w+)Impl\\s*,", 0, &errs, &erro, NULL);
  if (!pat) {
    fclose(f);
    fprintf(stderr, "ERROR compiling internal regex pattern (%s)\n", errs);
    exit(1);
  }

  while (fgets(line, sizeof(line), f)) {
    int vec[6];

    if (pcre_exec(pat, NULL, line, static_cast<int>(strlen(line)), 0, 0, vec, 6) == 2) {
      char buf[1024];
      pcre_copy_substring(line, vec, 2, 1, buf, sizeof(buf));
      interfaces.push_back(buf);
    }
  }

  fclose(f);
  do_generate_interface_classes(outfile, interfaces);
}

//----------------------------------------------------------------------------------------------------------------------

void generate_module_classes(const char *outpath) {
  puts("NOT IMPLEMENTED");
#if 0
  grt::GRT grt_;
  MYX_GRT *grt= grt_.grt();

  register_all_interfaces(&grt_);
  MYX_GRT_MODULE** list;
  int list_size= 0;
  
  list= g_new0(MYX_GRT_MODULE*, grt::GRT::get()->interfaces_num);

  for (unsigned int i= 0; i < grt::GRT::get()->interfaces_num; i++) {
    MYX_GRT_MODULE *intf= grt::GRT::get()->interfaces[i];

    if (interfaces.empty() || is_in_list(intf->name, interfaces)) {
      list[list_size++]= intf;
    }
  }

  if (list_size > 0) {
    MYX_GRT_ERROR err;
    
    if (!interfaces.empty() && interfaces.size() != list_size)
      fprintf(stderr, "WARNING: Some of the specified interfaces were not found\n");

    err= myx_grt_modules_export_wrapper(list, list_size, outfile);
    if (err != MYX_GRT_NO_ERROR) {
      fprintf(stderr, "Error generating wrappers: %s\n",
              myx_grt_error_string(err));
      exit(1);
    }
  } else
    fprintf(stderr, "No interfaces to be wrapped.\n");

  g_free(list);
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void help() {
  printf("genwrap <command> <options>\n");
  printf("Commands:\n");
  printf("  interfaces <source-header> <output-header-path>\n");
  printf("  wrappers <output-header-path>\n");
}

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv) {
  if (argc == 1) {
    help();
    return 1;
  }

  base::Logger(".");

  if (strcmp(argv[1], "interfaces") == 0) {
    if (argc < 4) {
      printf("bad # of arguments\n");
      exit(1);
    }
    generate_interface_classes(argv[2], argv[3]);
  } else if (strcmp(argv[1], "wrappers") == 0) {
    if (argc < 2) {
      printf("bad # of arguments\n");
      exit(1);
    }
    // generate_wrapper_classes(argv[2]);
  } else {
    printf("invalid command %s\n", argv[1]);
    exit(1);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
