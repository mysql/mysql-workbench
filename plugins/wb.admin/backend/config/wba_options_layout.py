import mforms
import option
#import options_layout

utest = False
if __name__ == "__main__":
    utest = True
    import pprint


#===============================================================================
#
#===============================================================================
# A mere container for some utility info
class WBAOptionLayoutRow(object):
    path    = None
    visible = None

    #---------------------------------------------------------------------------
    def __init__(self, path):
        self.path    = [path] if path is not None else []
        self.visible = True

    #---------------------------------------------------------------------------
    def get_paths_list(self):
        return self.path

    #---------------------------------------------------------------------------
    def get_paths_count(self):
        return len(self.path)

    #---------------------------------------------------------------------------
    def add_path(self, path):
        self.path.append(path)

    #---------------------------------------------------------------------------
    def drop_last_path(self):
        if len(self.path) > 0:
            self.path.pop()

    #---------------------------------------------------------------------------
    def get_last_path(self):
        return self.path[-1] if len(self.path) > 0 else None

    #---------------------------------------------------------------------------
    def get_path(self, n):
        return self.path[n] if (n >= 0 and n < len(self.path)) else None;

    #---------------------------------------------------------------------------
    def reset_path(self):
        self.path = []

#-------------------------------------------------------------------------------
def filter_layout(layout, filter_function):
    new_layout = []
    for tabname, tabcont in layout:
        newtabcont = []
        for groupname, groupcont in tabcont:
            newgroupcont = []
            for option in groupcont:
                if filter_function(option):
                    newgroupcont.append(option)
            if len(newgroupcont) > 0:
                newtabcont.append((groupname, newgroupcont))
        if len(newtabcont) > 0:
            new_layout.append((tabname, newtabcont))

    return new_layout

#===============================================================================
#
#===============================================================================
class Source:
    get_value               = None
    content_edited          = None
    ro_content_clicked      = None
    content_action          = None

    def __init__(self, value_cb, content_edited_cb = None, ro_content_clicked_cb = None, content_action_cb = None):
        self.get_value            = value_cb
        self.content_edited       = content_edited_cb
        self.ro_content_clicked   = ro_content_clicked_cb
        self.content_action       = content_action_cb

#===============================================================================
#
#===============================================================================
class WBAOptionsLayout(object):
    grid         = None # UI widget
    layout       = None # layout dict used to build current layout
    rows_index   = None # dict: Option name -> WBAOptionLayoutRow
    column_names = None # dict: column name -> column id
    sources      = None # List of entities which can return data based on (tag, row)
    postprocess  = None # Modifies styles, ro/rw state for a row/cell

    #---------------------------------------------------------------------------
    def __init__(self, grid, layout_dict, columns):
        object.__init__(self)
        self.grid = grid
        self.column_names = {}
        self.grid.add_content_edited_callback(self.on_content_edited)

        for column_name in columns:
            colid = self.grid.add_column(column_name);
            self.column_names[column_name] = colid

        self.sources = {}
        #[None] * max(self.column_names.values())
        self.rebuild_ui(layout_dict)

    #---------------------------------------------------------------------------
    def column_name(self, colid):
        ret = None
        for name, cid in self.column_names.iteritems():
            if cid == colid:
                ret = name
                break
        return ret

    #---------------------------------------------------------------------------
    def set_postprocess_call(self, postprocess):
        self.postprocess = postprocess

    #---------------------------------------------------------------------------
    def add_source(self, column_name, source):
        colid = self.column_names[column_name]
        self.sources[colid] = source
        return colid

    #---------------------------------------------------------------------------
    def copy_data_to_ui(self, columns_data, option_layout_row):
        for i, path in enumerate(option_layout_row.get_paths_list()):
            for (values_list, vtype, aux_param, colid, colname) in columns_data:
                self.grid.set_str_value(path, colid, "", False) # Clear cell, just to be on a safe side
                ptype = vtype.as_type()

                if i < len(values_list):
                    value = values_list[i]
                else:
                    value = None

                if ptype is type("") or ptype is type(1):
                    editable = aux_param
                    if value is None:
                        value = option.Value("", True)
                    elif value.value is None:
                        value.value = ""
                    self.grid.set_str_value(path, colid, str(value.value), editable)
                elif ptype is type(True):
                    editable = aux_param
                    if value is None:
                        value = option.Value(False, False)
                    self.grid.set_bool_value(path, colid, value.enabled, editable)
                elif ptype is type([]):
                    enum_list = aux_param
                    is_enum = type(enum_list) is list

                    if is_enum:
                        self.grid.set_enum(path, colid, enum_list)

                    if value is None:
                        value = ""
                    else:
                        value = value.value
                    self.grid.set_str_value(path, colid, value, True if is_enum else False)
                else:
                    print "Unhandled type '%s', %s for %s" % (str(vtype), str(vtype.as_type()), name)

                if self.postprocess:
                    self.postprocess(path, colid, colname)

    #---------------------------------------------------------------------------
    # In case internal repr was changed (the layout must remain untouched)
    # updates view
    def update_ui(self, option_name = None):
        # Build a list of tuples (path, name) sorted so that last paths are at the start
        # of the sorted list. This way we will not need to rebuild self.rows_index until
        # update_ui completes.
        if option_name is None:
            rows_list = [(name, row) for name, row in self.rows_index.iteritems()]
            rows_list.sort(key = lambda x: x[1].get_path(0), reverse=True)
        else:
            rows_list = [(option_name, self.rows_index.get(option_name))]

        # walk rows_index and request
        for name, option_layout_row in rows_list:
            if type(option_layout_row) is WBAOptionLayoutRow:
                # request data from each source and set values to self.grid accordingly
                columns_data = [] # we need to use data for a couple of times. save a local copy

                for column_name, colid in self.column_names.iteritems():
                    source = self.sources.get(colid)
                    if source:
                        values_list, vtype, aux_param = source.get_value(name)
                        if type(values_list) is not list:
                            values_list = [values_list]

                        columns_data.append((values_list, vtype, aux_param, colid, column_name))

                self.copy_data_to_ui(columns_data, option_layout_row)

    #---------------------------------------------------------------------------
    # The method resets current internal repr together with view. After that the
    # internal repr is rebuilt, the view is rebuilt accordingly.
    def rebuild_ui(self, new_layout_dict = None):
        # Clear mapped paths
        if new_layout_dict:
            self.layout = new_layout_dict # we may have simpler/shorted layout passed, e.g. for diff viewing
        self.rows_index = {} # as internal view repr will be rebuilt, we do not need old rows
        self.grid.clear(); # clear the grid itself

        for top_group_name, top_group_content in self.layout:
            group_path = self.grid.append_header(top_group_name)
            self.grid.set_row_tag(group_path, top_group_name)
            for group_name, group_items in top_group_content:
                if type(group_items) is list:
                    path = self.grid.append_row(group_path)
                    self.grid.set_row_caption(path, group_name) # That should be the data to display for the group header
                    self.grid.set_cell_type(path, -1, mforms.CellGroupHeader)
                    for item in group_items:
                        if type(item) is str or type(item) is unicode:
                            columns_data = []    # we need to use data for a couple of times. save a local copy
                            number_of_values = 0 # max number of values across columns

                            for column_name, colid in self.column_names.iteritems():
                                source = self.sources.get(colid)
                                if source:
                                    values_list, vtype, aux_param = source.get_value(item)
                                    if type(values_list) is not list:
                                        values_list = [values_list]

                                    columns_data.append((values_list, vtype, aux_param, colid, column_name))
                                    number_of_values = max(number_of_values, len(values_list))

                            layout_row = WBAOptionLayoutRow(None)
                            while number_of_values > 0:
                                path = self.grid.append_row(group_path)
                                self.grid.set_row_tag(path, item)
                                layout_row.add_path(path)
                                number_of_values -= 1

                            self.rows_index[item] = layout_row
                            self.copy_data_to_ui(columns_data, layout_row)
                elif type(item) is str or type(item) is unicode:
                    path = self.grid.append_row(group_path)
                    self.rows_index[item] = WBAOptionLayoutRow(path)

        print "Done rebuild"

    #---------------------------------------------------------------------------
    def on_content_edited(self, path, colid):
        option_name = self.grid.get_row_tag(path)
        print "on_edited_cb: '%i:%i - %s" % (path.index(0), path.index(1), option_name)
        rows = self.rows_index.get(option_name)
        src = self.sources[colid] if colid < len(self.sources) else None
        if src and src.content_edited and rows:
            src.content_edited(option_name, rows.path, colid)

    #---------------------------------------------------------------------------
    def on_ro_content_clicked(self, path, colid):
        pass

    #---------------------------------------------------------------------------
    def on_content_action(self, path, colid):
        pass

    #---------------------------------------------------------------------------
    def get_ui_state(self):
        state = {}
        exp_nodes = {}
        state["exp_nodes"] = exp_nodes

        path = mforms.GridPath()
        for i in range(0, self.grid.get_children_count(path)):
            path.append(i)
            tag = self.grid.get_row_tag(path)
            expanded = self.grid.is_node_expanded(path)
            exp_nodes[tag] = expanded
            path.up()

        return state

    #---------------------------------------------------------------------------
    def restore_ui_state(self, state):
        exp_nodes = state["exp_nodes"]
        path = mforms.GridPath()
        for i in range(0, self.grid.get_children_count(path)):
            path.append(i)
            tag = self.grid.get_row_tag(path)
            expanded = exp_nodes.get(tag, False)
            self.grid.set_node_expanded(path, expanded)
            path.up()

if utest:
    class mforms:
        CellGroupHeader = 5


    class TestGrid:
        def __init__(self):
            self.cols = 0
            self.storage = []

        def add_column(self, s):
            self.cols += 1
            return self.cols

        def clear(self):
            self.storage = []

        def append_header(self, v):
            self.storage.append((v, []))
            print "Appending header %s" % str(v)
            return (len(self.storage) - 1,)

        def append_row(self, v):
            group_name, group = self.storage[v[0]]
            group.append([])
            r = (v[0], len(group) - 1)
            print "Appending row to group path %s" % str(r)
            return r

        def set_row_tag(self, v, c):
            pass

        def set_cell_type(self, v, c, t):
            pass

    class DummySource:
        def get_value(self, name):
            return "Value of " + name

    olayout = [ ( 'General',
                        [ ('Networking', ['enable-named-pipe', 'port', 'skip-networking']),
                          ('Directories', ['datadir', 'basedir', 'tmpdir']),
                          ('Memory usage', ['sort_buffer_size']),
                        ]
               ),
               ( 'Advanced',
                        [
                          ( 'Localization', [ 'default-collation', 'default-time-zone', 'language' ] )
                        ]
               )
             ]

    grid = TestGrid()
    layout = WBAOptionsLayout(grid, olayout, ["Column1", "Column2"])
    layout.add_source(DummySource(), "Column1")
    layout.add_source(DummySource(), "Column2")
    layout.update_ui()
    layout.rebuild_ui(olayout)
