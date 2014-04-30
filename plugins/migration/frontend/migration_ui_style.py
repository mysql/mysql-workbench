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
import sys
import new

#-------------------------------------------------------------------------------
def newHeaderLabel(text):
    widget = mforms.newPanel(mforms.StyledHeaderPanel)
    label = mforms.newLabel(text)
    widget.add(label)
    widget.set_text  = new.instancemethod(mforms.Label.set_text,  label, mforms.Label)
    widget.set_color = new.instancemethod(mforms.Label.set_color, label, mforms.Label)
    widget.set_style = new.instancemethod(mforms.Label.set_style, label, mforms.Label)
    widget.set_text_align = new.instancemethod(mforms.Label.set_text_align, label, mforms.Label)
    return widget

#===============================================================================
#
#===============================================================================
class wbOS(object):
    unknown = "unknown"
    windows = "windows"
    linux   = "linux"
    darwin  = "darwin"

    def __setattr__(self, name, value):
        raise NotImplementedError

#===============================================================================
#
#===============================================================================
class UIProfile(object):
    def __init__(self):
        self.styles = {
                        wbOS.windows :
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
                               'subsection-label' : (lambda x: x.set_back_color("#d9e2ef"),
                                                     lambda x: x.set_color("#000000")
                                                    ),
                               'option-search-panel' : (lambda x: x.set_back_color("#bdc7de"),
                                                        lambda x: x.set_size(-1, 26)
                                                       )
                             },
                        wbOS.linux :
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
                                                  lambda x: x.set_size(-1, 24)
                                                 ),
                               'subsection-label' : (lambda x: x.set_back_color("#d9e2ef"),
                                                     lambda x: x.set_color("#000000")
                                                    ),
                               'option-search-panel' : (
                                                        lambda x: x.set_size(-1, 26),
                                                       )
                             },
                        wbOS.darwin :
                             {
                               'main' : (lambda x: x.set_spacing(1),
                                         lambda x: x.set_back_color("#bbbbbb")),
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

    def host_os(self):
        if hasattr(sys, 'getwindowsversion'):
            return wbOS.windows
        elif ('inux' in sys.platform):
            return wbOS.linux
        elif ('arwin' in sys.platform):
            return wbOS.darwin
        return wbOS.unknown

    #---------------------------------------------------------------------------
    def apply_style(self, target, style_name):
        style = None
        os_ui_profile = self.styles.get(self.host_os(), None)
        if os_ui_profile:
            style = os_ui_profile.get(style_name)

        if style is not None:
            for style_part in style:
                style_part(target)
        else:
            print "OS profile has no style '%s'" % style_name

    @staticmethod
    def newHeaderLabel(text):
        return newHeaderLabel(text)
