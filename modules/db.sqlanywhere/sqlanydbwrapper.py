# Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import sqlanydb

class WrappedConnect(object):
    def __init__(self, *args, **kwargs):
        self.conn = sqlanydb.connect(*args, **kwargs)

    def cursor(self):
        return WrappedCursor(self.conn.cursor())

    def close(self):
        self.conn.close()


class WrappedCursor(object):
    def __init__(self, cursor):
        self.cursor = cursor

    def cursor(self):
        return WrappedCursor(self.conn.cursor())

    def execute(self, operation, *args, **kwargs):
        self.cursor.execute(operation, *args, **kwargs)
        return self

    def __iter__(self):
        return self

    def __next__(self):
        return next(self.cursor.rows())

    def fetchone(self):
        return self.cursor.fetchone()

    def fetchmany(self, size=None):
        return self.cursor.fetchmany(size)

    def fetchall(self):
        return self.cursor.fetchall()

def connect(*args, **kwargs):
    return WrappedConnect(*args, **kwargs)
