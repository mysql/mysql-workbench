# Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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
        
        self.set_on_close(self.on_close)
     
    def on_close(self):
        current_page = self.tabview.get_active_tab()
        if hasattr(self.pages[current_page], "on_close"):
            return self.pages[current_page].on_close()
        return True


    def add_page(self, page):
        if self.pages:
            self.pages[len(self.pages) - 1].set_last_page(False)
        else:
            page.back_button.set_enabled(False)
        page.set_last_page(True)
        self.pages.append(page)
        self.tabview.add_page(page, "")


    def go_next_page(self):
        current_page = self.tabview.get_active_tab()
        if not self.pages[current_page].validate():
            return
        if current_page == len(self.pages) - 1:
            self.finish()
        for index in range(current_page + 1, len(self.pages)):
            if self.pages[index].should_skip():
                continue
            self.tabview.set_active_tab(index)
            self.pages[index].page_activated(True)
            break


    def go_previous_page(self):
        current_page = self.tabview.get_active_tab()
        for index in range(current_page - 1, -1, -1):
            if self.pages[index].should_skip():
                continue
            self.tabview.set_active_tab(index)
            self.pages[index].page_activated(False)
            break

    def finish(self):
        self.close()

    def cancel(self):
        self.end_modal(False)


    def run(self, modal = False):
        self.pages[0].page_activated(True)

        if modal:
            self.run_modal(None, None)
        else:
            self.show()



