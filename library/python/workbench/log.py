# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
import os
import traceback

def log_error(msg):    
    tb = traceback.extract_stack(limit=2)
    grt.log_error("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)

def log_warning(msg):
    tb = traceback.extract_stack(limit=2)
    grt.log_warning("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)

def log_info(msg):
    tb = traceback.extract_stack(limit=2)
    grt.log_info("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)

def log_debug(msg):
    tb = traceback.extract_stack(limit=2)
    grt.log_debug("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)

def log_debug2(msg):
    tb = traceback.extract_stack(limit=2)
    grt.log_debug2("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)

def log_debug3(msg):
    tb = traceback.extract_stack(limit=2)
    grt.log_debug3("%s:%s:%s"%(os.path.basename(tb[-2][0]),tb[-2][2],tb[-2][1]), msg)
