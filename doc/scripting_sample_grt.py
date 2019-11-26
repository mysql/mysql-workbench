
from wb import *
import grt

ModuleInfo = DefineModule(name= "MyModule", author= "My Name", version="1.0")

def process_script(script):
	import re
	import random
	tokens = re.split("(%\[.*?\]%)", script)
	output = []
	for token in tokens:
		out_token = token
		if token.startswith("%[") and token.endswith("]%"):
			command, sep, args = token[2:-2].partition(":")
			if command == "oneof":
				out_token = random.choice(args.split(","))
			elif command == "random":
				min_value, sep, max_value = args.partition(",")
				out_token = "%s" % random.randint(int(min_value), int(max_value))
		output.append(out_token)
	return "".join(output)


# because of a bug in the wbinputs.currentSQLEditor() input	specifier from the wb module 
# in Workbench 5.2.26, we include our own version of it here
def currentSQLEditor():
	arg= grt.classes.app_PluginObjectInput()
    	arg.name= "activeSQLEditor"
    	arg.objectStructName= "db.query.Editor"
    	return arg


@ModuleInfo.plugin("my.plugin.fill_random_query", caption= "Fill in Random Values in Query", description="Replaces %[oneof:opt1,opt2,opt3] and %[random:<min>,<max>]% in a query with a randomly picked value.", input=[currentSQLEditor()], pluginMenu="SQL/Utilities", accessibilityName="Fill Random Query")
@ModuleInfo.export(grt.INT, grt.classes.db_query_Editor)
def fill_random_query(editor):
	active_buffer = editor.activeQueryBuffer
	script = active_buffer.script

	try:
		new_script = process_script(script)	
	except Exception as exc:
		new_script = "Error: %s" % exc

	new_buffer = editor.addQueryBuffer()
	new_buffer.replaceContents(new_script)

	return 0

