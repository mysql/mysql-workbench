/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifdef __OBJC__
    #import <Cocoa/Cocoa.h>
#endif

#ifdef __cplusplus

#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <stack>

#include <sys/types.h>
#include <sys/stat.h>

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
#include <boost/enable_shared_from_this.hpp>

#ifndef EXCLUDE_MYSQL_SUPPORT
//#include <my_global.h> causes trouble with boost interprocess.
#include <mysql.h>
//#ifdef bool
//#undef bool
//#endif
#endif

#include "test.h"

#ifndef TEST_STANDALONE
#include "wb_helpers.h"
#endif

// Common headers.
#include <antlr3.h>

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.db.migration.h"

#endif
