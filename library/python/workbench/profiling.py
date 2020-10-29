# Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

from datetime import datetime

class TimeProfiler(object):
    """
    Usage of this class is intended to measure the time spent during a function execution
    in any python class.

    To use it you just need to make the target class inherit from this one.

    Result will be printed output indicating whether a call to a function started and finished.

    When a function ends the output will also contain the execution duration in the format of:
    Minutes:Seconds.Microseconds

    The --> Begin and --> End forman can be used format the log nesting it for better understanding.
    (However it is not yet ready for multithreading.)
    """
    def __getattribute__(self,name):
        attr = object.__getattribute__(self, name)
        classobj = object.__getattribute__(self, '__class__')

        if hasattr(attr, '__call__'):
            methodname = ''
            if hasattr(classobj, '__name__'):
                methodname = '%s.%s' % (classobj.__name__, name)
            else:
                methodname = name

            def newfunc(*args, **kwargs):
                startt = datetime.now()
                print(('--> Begin: %s' % methodname))
                result = attr(*args, **kwargs)
                endtt = datetime.now()
                deltat= endtt - startt
                minutes = deltat.seconds / 60
                seconds = deltat.seconds - (minutes * 60)
                print(('--> End: %s [%s:%s.%s]' % (methodname, minutes, seconds, deltat.microseconds)))
                return result
            return newfunc
        else:
            return attr