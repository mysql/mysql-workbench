# import the wb module
from wb import DefineModule, wbinputs
# import the grt module
import grt
# import the mforms module for GUI stuff
import mforms

from sql_reformatter import node_value, node_symbol, node_children, find_child_node, find_child_nodes, trim_ast, ASTHelper, dump_tree, flatten_node

# define this Python module as a GRT module
ModuleInfo = DefineModule(name= "CodeUtils", author= "Oracle Corp.", version="1.0")


@ModuleInfo.plugin("wb.sqlide.copyAsPHPConnect", caption= "Copy as PHP Code (Connect to Server)", input= [wbinputs.currentSQLEditor()], pluginMenu= "SQL/Utilities")
@ModuleInfo.export(grt.INT, grt.classes.db_query_Editor)
def copyAsPHPConnect(editor):
    """Copies PHP code to connect to the active MySQL connection to the clipboard.
    """
    if editor.connection:
        conn = editor.connection
        
        if conn.driver.name == "MysqlNativeSocket":
            params = {
            "host" : "p:localhost",
            "port" : 3306,
            "user" : conn.parameterValues["userName"],
            "socket" : conn.parameterValues["socket"],
            "dbname" : editor.defaultSchema
            }
        else:
            params = {
            "host" : conn.parameterValues["hostName"],
            "port" : conn.parameterValues["port"] if conn.parameterValues["port"] else 3306,
            "user" : conn.parameterValues["userName"],
            "socket" : "",
            "dbname" : editor.defaultSchema
            }

        text = """$host="%(host)s";
$port=%(port)s;
$socket="%(socket)s";
$user="%(user)s";
$password="";
$dbname="%(dbname)s";

$con = new mysqli($host, $user, $password, $dbname, $port, $socket)
	or die ('Could not connect to the database server' . mysqli_connect_error());

//$con->close();
""" % params
        mforms.Utilities.set_clipboard_text(text)
        mforms.App.get().set_status_text("Copied PHP code to clipboard")
    return 0


def _parse_column_name_list_from_query(query):
    from grt.modules import MysqlSqlFacade

    ast_list = MysqlSqlFacade.parseAstFromSqlScript(query)
    for ast in ast_list:
        if type(ast) is str:
            continue
        else:
            s, v, c, _base, _begin, _end = ast
            trimmed_ast = trim_ast(ast)
            select_item_list = find_child_node(trimmed_ast, "select_item_list")
            if select_item_list:
                columns = []
                variables = []
                index = 0
                for node in node_children(select_item_list):
                    if node_symbol(node) == "select_item":
                        alias = find_child_node(find_child_node(node, "select_alias"), "ident")
                        if not alias:
                            ident = find_child_node(node, "simple_ident_q")
                            if ident and len(node_children(ident)) == 3:
                                ident = node_children(ident)[-1]
                            else:
                                ident = find_child_node(find_child_node(node, "expr"), "ident")
                            if ident:
                                name = node_value(ident)
                            else:
                                name = "field"
                                field = flatten_node(node)
                                if field:
                                    import re
                                    m = re.match("([a-zA-Z0-9_]*)", field)
                                    if m:
                                        name = m.groups()[0]
                        else:
                            name = node_value(alias)
                        columns.append(name)

                helper = ASTHelper(query)
                begin, end = helper.get_ast_range(ast)
                #dump_tree(sys.stdout, ast)
                
                query = query[begin:end]
                offset = begin
                
                vars = find_child_nodes(ast, "variable")
                for var in reversed(vars):
                    begin, end = helper.get_ast_range(var)
                    begin -= offset
                    end -= offset
                    
                    name = query[begin:end]
                    query = query[:begin] + "?" + query[end:]
                    variables.insert(0, name)

                duplicates = {}
                for i, c in enumerate(columns):
                    if duplicates.has_key(c):
                        columns[i] = "%s%i" % (c, duplicates[c])
                        duplicates[c] += 1
                    duplicates[c] = duplicates.get(c, 0)+1

                return query, columns, variables


@ModuleInfo.plugin("wb.sqlide.copyAsPHPQueryAndFetch", caption= "Copy as PHP Code (Iterate SELECT Results)", input= [wbinputs.currentQueryBuffer()], pluginMenu= "SQL/Utilities")
@ModuleInfo.export(grt.INT, grt.classes.db_query_QueryBuffer)
def copyAsPHPQueryAndFetch(qbuffer):
    """Copies PHP code to execute the query and iterate its results to the clipboard. The code will substitute @variables with
parameter markers (?) and will bind them with matching PHP variables. The results will be bound to PHP variables matching
the SELECTed column names or their aliases.
    """
    sql= qbuffer.selectedText or qbuffer.script

    # try to parse the query and extract the columns it returns
    res = _parse_column_name_list_from_query(sql)
    if res:
        sql, column_names, variable_names = res
    else:
        column_names = None
        variable_names = None
    sql = sql.replace("\r\n", " ").replace("\n", " ").strip()
    
    if sql.endswith(";"):
        sql = sql[:-1]
    
    variable_assignments = ""
    variable_bind = ""
    if variable_names:
        variable_assignments = "\n".join(["$%s = '';" % var[1:] for var in variable_names]) + "\n"
        variable_bind = "$stmt->bind_param('%s', %s); //FIXME: param types: s- string, i- integer, d- double, b- blob\n" % ("s"*len(variable_names), ", ".join(["$"+var[1:] for var in variable_names]))
    
    if not column_names:
        column_names = ["field1", "field2"]

    text = """$query = "%(query)s";
%(vars)s
%(var_bind)s
if ($stmt = $con->prepare($query)) {
    $stmt->execute();
    $stmt->bind_result(%(column_list)s);
    while ($stmt->fetch()) {
        //printf("%(fmt_list)s\\n", %(column_list)s);
    }
    $stmt->close();
}""" % { "query" : sql.replace('"', r'\"'), 
        "column_list" : ", ".join(["$%s"%c for c in column_names]), "fmt_list" : ", ".join(["%s"]*len(column_names)),
        "vars" : variable_assignments,
        "var_bind" : variable_bind}

    mforms.Utilities.set_clipboard_text(text)

    mforms.App.get().set_status_text("Copied PHP code to clipboard")
    return 0

