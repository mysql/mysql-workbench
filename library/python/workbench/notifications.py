# Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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


class NotificationCenter(object):
    __instance = None
    observers = []

    def __new__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super(NotificationCenter, cls).__new__(cls, *args, **kwargs)
        return cls.__instance

    def _the_observer(self, name, sender, args):
        for obs, nam, obj in self.observers:
            if (nam is None or name == nam) and (obj is None or obj == sender):
                try:
                    obs(name, sender, args)
                except:
                    grt.log_error("PyNotificationCenter", "Error calling notification observer for %s\n" % name)
                    import traceback
                    traceback.print_exc()

    def add_observer(self, observer, name = None, object = None):
        self.observers.append((observer, name, object))

    def remove_observer(self, observer, name = None):
        for i, (obs, n, obj) in enumerate(self.observers):
            if observer == obs and (name is None or name == n):
                del self.observers[i]
                break

    def send(self, name, sender, info):
        grt.send_grt_notification(name, sender, info)

nc = NotificationCenter()
grt._set_grt_notification_observer(nc._the_observer)
