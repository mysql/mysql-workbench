/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifdef __OBJC__
    #import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <algorithm>
#include <stack>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include <iconv.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <errno.h>
#include <stdio.h>

#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>

#include "tut.h"
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
//#include <gl.h>

#include <glib.h>

#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>

#ifndef EXCLUDE_MYSQL_SUPPORT
//#include <my_global.h> causes trouble with boost interprocess.
#include <mysql.h>
//#ifdef bool
//#undef bool
//#endif
#endif

#include "test.h"

// Common headers.
#include <antlr4-runtime.h>

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.migration.h"

#endif
