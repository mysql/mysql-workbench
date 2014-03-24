# Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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
import mforms
from wb_server_management import wbaOS

#===============================================================================
#
#===============================================================================
class UIProfile(object):
    def __init__(self, server_profile):
        self.server_profile = server_profile
        self.host_os = server_profile.host_os
        self.styles = {
                        wbaOS.windows :
                             {
                               'main' : (lambda x: x.set_back_color("#293852"),
                                         lambda x: x.set_spacing(6)
                                        ),
                               'page' : (lambda x: x.set_back_color("#ffffff"),),
                               'sidebar-label' : (lambda x: x.set_back_color("#4a6184"),
                                                  lambda x: x.set_color("#ffffff"),
                                                  lambda x: x.set_size(-1, 25)
                                                 ),
                               'content-label' : (lambda x: x.set_back_color("#4a6184"),
                                                  lambda x: x.set_color("#ffffff"),
                                                  lambda x: x.set_size(-1, 25)
                                                 ),
                               'subsection-label' : (lambda x: x.set_back_color("#ffffff"),
                                                     lambda x: x.set_color("#000000")
                                                    ),
                               'option-search-panel' : (lambda x: x.set_back_color("#bdc7de"),
                                                        lambda x: x.set_size(-1, 26)
                                                       )
                             },
                        wbaOS.linux :
                             {
                               'main' : (lambda x: x.set_padding(0),
                                         lambda x: x.set_spacing(1)
                                        ),
                               'page' : (lambda x: None,),
                               'sidebar-label' : (lambda x: x.set_color("#ffffff"),
                                                  lambda x: x.set_style(mforms.BoldStyle),
                                                  lambda x: x.set_size(-1, 24),
                                                 ),
                               'content-label' : (lambda x: x.set_color("#ffffff"),
                                                  lambda x: x.set_style(mforms.BoldStyle),
                                                  lambda x: x.set_size(-1, 20)
                                                 ),
                               'subsection-label' : (lambda x: x.set_back_color("#d9e2ef"),
                                                     lambda x: x.set_color("#000000")
                                                    ),
                               'option-search-panel' : (
                                                        lambda x: x.set_size(-1, 26),
                                                       )
                             },
                        wbaOS.darwin :
                             {
                               'main' : (lambda x: x.set_spacing(1),
                                         ),
                               'page' : (lambda x: None, ),
                               'sidebar-label' : (lambda x: x.set_back_color("#efefef"),
                                                  lambda x: x.set_color("#454545"),
                                                  lambda x: x.set_size(-1, 25)
                                                 ),
                               'content-label' : (lambda x: x.set_color("#454545"),
                                                  lambda x: x.set_style(mforms.SmallBoldStyle),
                                                  lambda x: x.set_text_align(mforms.MiddleCenter),
                                                  lambda x: x.set_size(-1, 22)
                                                 ),
                               'subsection-label' : (lambda x: x.set_style(mforms.SmallBoldStyle),
                                                     lambda x: x.set_color("#484950")
                                                    ),
                               'option-search-panel' : (lambda x: x.set_back_color("#f1f1f1"),
                                                        lambda x: x.set_size(-1, 26)
                                                       )
                             }
                      }

    #---------------------------------------------------------------------------
    def apply_style(self, target, style_name):
        style = None
        os_ui_profile = self.styles.get(self.host_os, None)
        if os_ui_profile:
            style = os_ui_profile.get(style_name)

        if style is not None:
            for style_part in style:
                style_part(target)
        else:
            print "OS profile has no style '%s'" % style_name
