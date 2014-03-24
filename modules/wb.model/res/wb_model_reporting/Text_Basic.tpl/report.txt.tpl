+--------------------------------------------+
|  {{TITLE}}                                 |
+--------------------------------------------+

Total number of Schemata: {{SCHEMA_COUNT}}
=============================================
{{#SCHEMATA}}
{{SCHEMA_NR}}. Schema: {{SCHEMA_NAME}}
----------------------------------------------
## Tables ({{TABLE_COUNT}}) ##
{{#TABLES}}{{TABLE_NR_FMT}}. Table: {{TABLE_NAME}}
{{#COLUMNS_LISTING}}## Columns ##
	Key	Column Name	Datatype	Not Null	Default	Comment
{{#COLUMNS}}	{{COLUMN_KEY}}	{{COLUMN_NAME}}	{{COLUMN_DATATYPE}}	{{COLUMN_NOTNULL}}	{{COLUMN_DEFAULTVALUE}}	{{COLUMN_COMMENT}}
{{/COLUMNS}}{{/COLUMNS_LISTING}}
{{#INDICES_LISTING}}## Indices ##
	Index Name	Columns	Primary	Unique	Type	Kind	Comment
{{#INDICES}}	{{INDEX_NAME}}	{{#INDICES_COLUMNS}}{{INDEX_COLUMN_NAME}}	{{INDEX_COLUMN_ORDER}}	{{INDEX_COLUMN_COMMENT}}{{/INDICES_COLUMNS}}	{{INDEX_PRIMARY}}	{{INDEX_UNIQUE}}	{{INDEX_TYPE}}	{{INDEX_KIND}}	{{INDEX_COMMENT}}
{{/INDICES}}{{/INDICES_LISTING}}
{{#REL_LISTING}}## Relationships ##
	Relationship Name	Relationship Type	Parent Table	Child Table	Cardinality
{{#REL}}	{{REL_NAME}}	{{REL_TYPE}}	{{REL_PARENTTABLE}}	{{REL_CHILDTABLE}}	{{REL_CARD}}
{{/REL}}{{/REL_LISTING}}
---------------------------------------------

{{/TABLES}}
{{/SCHEMATA}}
=============================================
End of MySQL Workbench Report
