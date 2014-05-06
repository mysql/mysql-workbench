 # iterate through all schemas
 for schema in grt.root.wb.doc.physicalModels[0].catalog.schemata:
     print schema.name

 # iterate through all tables from schema
 schema = grt.root.wb.doc.physicalModels[0].catalog.schemata[0]
 for table in schema.tables:
     print table.name

 # iterate through columns from schema
 schema = grt.root.wb.doc.physicalModels[0].catalog.schemata[0]
 for table in schema.tables:
     for column in table.columns:
         print table.name, column.name

 # iterate through all figures of a diagram
 diagram = grt.root.wb.doc.physicalModels[0].diagram
 for figure in diagram.figures:
     print figure.__grtclassname__, figure.left, figure.top

 # iterate through all resultset rows
 resultset = grt.root.wb.sqlEditors[0].activeResultset
 flag = resultset.goToFirst()
 while flag:
   print resultset.stringFieldValue(0)
   flag = resultset.nextRow()

 # replace sql editor contents
 editor = grt.root.wb.sqlEditors[0].activeQueryBuffer
 new_text = editor.sql
 editor.replaceContents(new_text)
