%pythoncode %{



class TreeView(TreeNodeView):
    """Compatibility class for old TreeView"""

    def set_string(self, row, col, value):
        if self.get_column_type(col) == IconColumnType:
            self.node_at_row(row).set_icon_path(col, value)
        else:
            self.node_at_row(row).set_string(col, value)
    
    def set_int(self, row, col, value):
        self.node_at_row(row).set_int(col, value)

    def set_bool(self, row, col, value):
        self.node_at_row(row).set_bool(col, value)

    def set_row_tag(self, row, value):
        self.node_at_row(row).set_tag(value)

    set_check = set_bool

    def get_row_tag(self, row):
        return self.node_at_row(row).get_tag()

    def get_string(self, row, col):
        return self.node_at_row(row).get_string(col)

    def get_int(self, row, col):
        return self.node_at_row(row).get_int(col)

    def get_bool(self, row, col):
        return self.node_at_row(row).get_bool(col)

    get_check = get_bool

    def get_selected(self):
        return self.get_selected_row()

    def set_selected(self, row):
        self.select_node(self.node_at_row(row))

    def add_row(self):
        return self.row_for_node(self.add_node())

    def clear_rows(self):
        self.clear()

    def delete_row(self, row):
        node = self.node_at_row(row)
        if node:
            node.remove_from_parent()

    def set_cell_edited_callback(self, callback):
        TreeNodeView.set_cell_edited_callback(self, self.cell_edited)
        self._cell_edited = callback

    def cell_edited(self, node, col, value):
        if self._cell_edited:
            return self._cell_edited(self.row_for_node(node), col, value)


def newTreeView(options):
    return TreeView(options|TreeFlatList)



#class ListBox(TreeNodeView):
#    """Compatibility class for old ListBox"""

%}


