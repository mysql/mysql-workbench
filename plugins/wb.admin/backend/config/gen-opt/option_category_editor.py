


import mforms
from mforms import newBox, newButton, newTreeNodeView, newTextEntry

import sys
sys.path.append("/tmp")


import variable_groups



class VariablesGrouper(mforms.Box):
  def __init__(self, variables, all_opts, output_path):
    mforms.Box.__init__(self, False)
    self.set_managed()
    self.set_release_on_add()
    
    self.suspend_layout()
    self.output_path = output_path
    
    box = newBox(True)
    box.set_spacing(12)
    self.add(box, True, True)
    self.tree = newTreeNodeView(mforms.TreeFlatList)
    self.tree.set_size(220, -1)
    
    sidebox = newBox(False)
    
    box.add_end(sidebox, False, True)
    
    sidebox.set_spacing(12)
    sidebox.add(self.tree, True, True)
    
    self.tree.add_column(mforms.CheckColumnType, "", 20, True)
    self.tree.add_column(mforms.StringColumnType, "", 160, False)
    self.tree.end_columns()
    self.tree.set_cell_edited_callback(self.toggled_group)
    
    self.values = newTreeNodeView(mforms.TreeFlatList)
    box.add(self.values, True, True)
    
    self.values.add_column(mforms.StringColumnType, "Name", 200, False)
    self.values.add_column(mforms.StringColumnType, "Groups", 1000, False)
    self.values.set_selection_mode(mforms.TreeSelectMultiple)
    self.values.end_columns()
    self.values.set_allow_sorting(True)
    self.values.add_changed_callback(self.value_selected)
    
    box = newBox(True)
    box.set_spacing(8)
    button = newButton()
    box.add_end(button, False, True)
    button.set_text("Save")
    button.add_clicked_callback(self.save)

    button = newButton()
    box.add_end(button, False, True)
    button.set_text("Add Group")
    button.add_clicked_callback(self.add_group)
    
    self.group_entry = newTextEntry()
    self.group_entry.set_size(100, -1)
    box.add_end(self.group_entry, False, True)

    
    box.set_padding(12)
    
    self.add(box, False, True)
    
    self.resume_layout()

    all_groups = set()
    vars = dict()
    for name, groups in variables:
        all_groups = all_groups.union(set(groups))
        vars[name] = groups

    for opt in all_opts:
        node = self.values.add_node()
        node.set_string(0, opt['name'])
        node.set_string(1, ", ".join(vars.get(opt['name'], [])))

    for g in sorted(all_groups):
        node = self.tree.add_node()
        node.set_string(1, g)
    self.groups = all_groups
  
  def value_selected(self):
      node = self.values.get_selected_node()
      if node:
        var = node.get_string(1)
        groups = var.split(",")
        self.tree.clear()
        for x in sorted(self.groups):
            x = x.strip()
            node = self.tree.add_node()
            node.set_string(1, x)
            node.set_bool(0, x in groups)
      
  
  def toggled_group(self, node, column, value):
      g = node.get_string(1)
      for sel in self.values.get_selection():
          var = [x for x in sel.get_string(1).strip().split(",") if x]
          node.set_bool(0, int(value))
          if int(value):
              sel.set_string(1, ",".join(sorted(var+[g])))
          else:
              var.remove(g)
              sel.set_string(1, ",".join(sorted(var)))

  
  def add_group(self):
    v = self.group_entry.get_string_value()
    self.groups.add(v)
    self.value_selected()
  

  def save(self):
    l = []
    for x in range(self.values.count()):
      n = self.values.node_at_row(x)
      l.append((n.get_string(0).encode("latin1"), [x.encode("latin1").strip() for x in n.get_string(1).split(",") if x]))
    import pprint
    f =open(self.output_path, "w+")
    f.write("variable_groups=")
    pp = pprint.PrettyPrinter(indent=2, stream=f)
    pp.pprint(l)
    f.close()


import raw_opts

form = mforms.Form(None, 0)
form.set_size(800,600)
grouper = VariablesGrouper(variable_groups.variable_groups, raw_opts.ropts, "/tmp/variable_groups.py")
form.set_content(grouper)

form.show()
