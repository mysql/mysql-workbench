<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>{{TITLE}} - Table Details</title>
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>
  
<body class="tbl_detail_page">
{{#SCHEMATA}}

  <a name="Schema_{{SCHEMA_ID}}">
    <div class="schema_header">Schema {{SCHEMA_NAME}} <div class="small_text">({{SCHEMA_NUMBER}}/{{SCHEMA_COUNT}})</div></div>
  </a>
{{#DDL_LISTING}}
  <div class="table_body">
    <div class="subitem_header">DDL script</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="" width="100%">
      <tr>
        <td class="ddl_field"><pre>{{DDL_SCRIPT}}</pre></td>
      </tr>
    </table>
  </div>
{{/DDL_LISTING}}
{{#TABLES}}
  <a name="Table_{{TABLE_ID}}">
    <div class="table_header">Table {{TABLE_NAME}} <div class="small_text">({{TABLE_NUMBER}}/{{TABLE_COUNT}})</div></div>
  </a>
  <div class="table_body">
	
	<!-- Table properties -->
	<div class="subitem_header">Table Properties</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
     <colgroup>
        <col width="20%" />
        <col width="*" />
        <col width="20%" />
        <col width="*" />
     </colgroup>
    <tr>
      <td class="details-property-name">Average Row Length</td>
      <td class="details-property-value">{{TABLE_AVG_ROW_LENGTH}}</td>
      <td class="details-property-name">Use Check Sum</td>
      <td class="details-property-value">{{TABLE_USE_CHECKSUM}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Connection String</td>
      <td class="details-property-value">{{TABLE_CONNECTION_STRING}}</td>
      <td class="details-property-name">Default Character Set</td>
      <td class="details-property-value">{{TABLE_CHARSET}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Default Collation</td>
      <td class="details-property-value">{{TABLE_COLLATION}}</td>
      <td class="details-property-name">Delay Key Updates</td>
      <td class="details-property-value">{{TABLE_DELAY_KEY_UPDATES}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Minimal Row Count</td>
      <td class="details-property-value">{{TABLE_MIN_ROW_COUNT}}</td>
      <td class="details-property-name">Maximum Row Count</td>
      <td class="details-property-value">{{TABLE_MAX_ROW_COUNT}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Union Tables</td>
      <td class="details-property-value">{{TABLE_UNION_TABLES}}</td>
      <td class="details-property-name">Merge Method</td>
      <td class="details-property-value">{{TABLE_MERGE_METHOD}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Pack Keys</td>
      <td class="details-property-value">{{TABLE_PACK_KEYS}}</td>
      <td class="details-property-name">Has Password</td>
      <td class="details-property-value">{{TABLE_HAS_PASSWORD}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Data Directory</td>
      <td class="details-property-value">{{TABLE_DATA_DIR}}</td>
      <td class="details-property-name">Index Directory</td>
      <td class="details-property-value">{{TABLE_INDEX_DIR}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Engine</td>
      <td class="details-property-value">{{TABLE_ENGINE}}</td>
      <td class="details-property-name">Row Format</td>
      <td class="details-property-value">{{TABLE_ROW_FORMAT}}</td>
    </tr>
  </table>

{{#PARTITION_LISTING}}
  <div class="subitem_header">Partitions</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
     <colgroup>
        <col width="20%" />
        <col width="*" />
        <col width="50%" />
        <col width="*" />
     </colgroup>
    <tr>
      <td class="subitem_table_head">Kind</td>
      <td class="subitem_table_head">Type</td>
      <td class="subitem_table_head">Parameters</td>
      <td class="subitem_table_head">Count</td>
    </tr>
    <tr>
      <td class="details-property-name">Partition</td>
      <td class="subitem_table_field">{{PARTITION_TYPE}}</td>
      <td class="subitem_table_field">{{PARTITION_EXPRESSION}}</td>
      <td class="subitem_table_field">{{PARTITION_COUNT}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Sub Partition</td>
      <td class="subitem_table_field">{{PARTITION_SUB_TYPE}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_EXPRESSION}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_COUNT}}</td>
    </tr>
  </table>
  <div class="subitem_header">Partition Definitions</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
    <tr>
      <td class="subitem_table_head">Partition</td>
      <td class="subitem_table_head">Value</td>
      <td class="subitem_table_head">Data Directory</td>
      <td class="subitem_table_head">Index Directory</td>
      <td class="subitem_table_head">Min Rows</td>
      <td class="subitem_table_head">Max Rows</td>
      <td class="subitem_table_head">Comment</td>
    </tr>
{{#PARTITIONS}}
    <tr>
      <td class="subitem_table_field">{{PARTITION_NAME}}</td>
      <td class="subitem_table_field">{{PARTITION_VALUE}}</td>
      <td class="subitem_table_field">{{PARTITION_DATA_DIR}}</td>
      <td class="subitem_table_field">{{PARTITION_INDEX_DIR}}</td>
      <td class="subitem_table_field">{{PARTITION_MIN_ROW_COUNT}}</td>
      <td class="subitem_table_field">{{PARTITION_MAX_ROW_COUNT}}</td>
      <td class="subitem_table_field">{{PARTITION_COMMENT}}</td>
    </tr>
{{#PARTITION_SUB_PARTITIONS}}
    <tr>
      <td class="details-sub-property-value">{{PARTITION_SUB_NAME}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_VALUE}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_DATA_DIR}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_INDEX_DIR}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_MIN_ROW_COUNT}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_MAX_ROW_COUNT}}</td>
      <td class="subitem_table_field">{{PARTITION_SUB_COMMENT}}</td>
    </tr>
{{/PARTITION_SUB_PARTITIONS}}

{{/PARTITIONS}}
  </table>
{{/PARTITION_LISTING}}
	
{{#COLUMNS_LISTING}}
  <div class="subitem_header">Columns</div>
  
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
    <tr>
      <td class="subitem_table_head">Key</td>
      <td class="subitem_table_head">Column Name</td>
      <td class="subitem_table_head">Datatype</td>
      <td class="subitem_table_head">Not Null</td>
      <td class="subitem_table_head">Default</td>
      <td class="subitem_table_head">Comment</td>
    </tr>
{{#COLUMNS}}
    <tr>
      <td class="subitem_table_field">{{COLUMN_KEY}}</td>
      <td class="subitem_table_field">{{COLUMN_NAME}}</td>
      <td class="subitem_table_field">{{COLUMN_DATATYPE}}</td>
      <td class="subitem_table_field">{{COLUMN_NOTNULL}}</td>
      <td class="subitem_table_field">{{COLUMN_DEFAULTVALUE}}</td>
      <td class="subitem_table_field">{{COLUMN_COMMENT}}</td>
    </tr>
{{/COLUMNS}}
  </table>
{{/COLUMNS_LISTING}}
{{#INDICES_LISTING}}
  <div class="subitem_header">Indices</div>
  
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
    <tr>
      <td class="subitem_table_head">Index Name</td>
      <td class="subitem_table_head">Columns</td>
      <td class="subitem_table_head">Primary</td>
      <td class="subitem_table_head">Unique</td>
      <td class="subitem_table_head">Type</td>
      <td class="subitem_table_head">Kind</td>
      <td class="subitem_table_head">Comment</td>
    </tr>
{{#INDICES}}
    <tr>
      <td class="subitem_table_field">{{INDEX_NAME}}</td>
      <td class="subitem_table_field"><table border="0" cellpadding="2" cellspacing="0" width="100%">
{{#INDICES_COLUMNS}}
          <tr>
            <td class="subitem_table_field">{{INDEX_COLUMN_NAME}}</td>
            <td class="subitem_table_field">{{INDEX_COLUMN_ORDER}}</td>
            <td class="subitem_table_field">{{INDEX_COLUMN_COMMENT}}</td>
          </tr>
{{/INDICES_COLUMNS}}
        </table></td>
      <td class="subitem_table_field">{{INDEX_PRIMARY}}</td>
      <td class="subitem_table_field">{{INDEX_UNIQUE}}</td>
      <td class="subitem_table_field">{{INDEX_TYPE}}</td>
      <td class="subitem_table_field">{{INDEX_KIND}}</td>
      <td class="subitem_table_field">{{INDEX_COMMENT}}</td>
    </tr>
{{/INDICES}}
  </table>
{{/INDICES_LISTING}}
{{#REL_LISTING}}
  <div class="subitem_header">Relationships</div>
  
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="" width="100%">
    <tr>
      <td class="subitem_table_head">Relationship Name</td>
      <td class="subitem_table_head">Relationship Type</td>
      <td class="subitem_table_head">Parent Table</td>
      <td class="subitem_table_head">Child Table</td>
      <td class="subitem_table_head">Card.</td>
    </tr>
{{#REL}}
    <tr>
      <td class="subitem_table_field">{{REL_NAME}}</td>
      <td class="subitem_table_field">{{REL_TYPE}}</td>
      <td class="subitem_table_field">{{REL_PARENTTABLE}}</td>
      <td class="subitem_table_field">{{REL_CHILDTABLE}}</td>
      <td class="subitem_table_field">{{REL_CARD}}</td>
    </tr>
{{/REL}}
  </table>
{{/REL_LISTING}}

{{#TABLE_COMMENT_LISTING}}
  <div class="subitem_header">Table Comments</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="" width="100%">
    <tr>
      <td class="subitem_table_field">{{TABLE_COMMENT}}</td>
    </tr>
  </table>
{{/TABLE_COMMENT_LISTING}}
{{#DDL_LISTING}}
  <div class="subitem_header">DDL script</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="" width="100%">
    <tr>
      <td class="ddl_field"><pre>{{DDL_SCRIPT}}</pre></td>
    </tr>
  </table>
{{/DDL_LISTING}}
  </div>
{{/TABLES}}

{{/SCHEMATA}}
</body>
</html>
