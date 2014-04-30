import random
import mforms
import raw_opts
import options_layout
import wba_config_file_be

class NameSource(object):
    def get_typed_value(self, name):
        return (name, mforms.CellText, False)

class ValueSource(object):
    def get_typed_value(self, name):
        if name[1] in ['a','b','c','d']:
            lst = ["1", "2", "3", "4", "5", "6"]
            return (lst[random.randint(0, len(lst) - 1)], mforms.CellEnum, lst)
        else:
            return (name, mforms.CellText, True)

frm = mforms.Form(None, mforms.FormResizable)

grid = mforms.newSimpleGrid()

layout = wba_config_file_be.WBAOptionsLayout(grid, options_layout.layout, ["Option name", "Option value"])

layout.add_source(NameSource(), "Option name")
layout.add_source(ValueSource(), "Option value")

layout.rebuild_ui(options_layout.layout)
layout.update_ui()

frm.set_content(grid)
frm.set_size(700,600)
frm.show()
