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


import mforms

class WizardForm(mforms.Form):
    def __init__(self, owner_form):
        mforms.Form.__init__(self, owner_form)

        self.pages = []

        self.content = mforms.newBox(False)
        self.content.set_spacing(12)

        hbox = mforms.newBox(True)
        self.header = mforms.newLabel("")
        self.header.set_style(mforms.WizardHeadingStyle)
        hbox.add(self.header, True, True)
        hbox.set_padding(24)

        self.content.add(hbox, False, True)
        self.tabview = mforms.newTabView(mforms.TabViewTabless)
        self.content.add(self.tabview, True, True)

        self.set_content(self.content)
        self.set_size(800, 600)
        self.center()


    def add_page(self, page):
        if not self.pages:
            page.back_button.set_enabled(False)
        self.pages.append(page)
        self.tabview.add_page(page, "")


    def go_next_page(self):
        current_page = self.tabview.get_active_tab()
        if not self.pages[current_page].validate():
            return
        if current_page < len(self.pages)-1:
            self.tabview.set_active_tab(current_page + 1)
            self.pages[current_page + 1].page_activated(True)


    def go_previous_page(self):
        current_page = self.tabview.get_active_tab()
        if current_page > 0:
            self.tabview.set_active_tab(current_page-1)
            self.pages[current_page].page_activated(False)


    def cancel(self):
        self.end_modal(False)


    def run(self):
        self.pages[0].page_activated(True)

        #self.run_modal(None, None)
        self.show()



