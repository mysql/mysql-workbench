<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>
  
<body class="full_detail_page">
{{#SCHEMATA}}
{{#COLUMNS}}
  <a name="Column_{{COLUMN_ID}}">
    <div class="table_header">Column {{COLUMN_NAME}}</div>
  </a>
  <div class="table_body">
    <div class="subitem_header">General Info</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
       <colgroup>
          <col width="20%" />
          <col width="*" />
       </colgroup>
      <tr>
        <td class="details-property-name">Schema Name</td>
        <td class="details-property-value">{{SCHEMA_NAME}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Table Name</td>
        <td class="details-property-value">{{TABLE_NAME}}</td>
      </tr>
       <tr>
        <td class="details-property-name">Key Part</td>
        <td class="details-property-value">{{COLUMN_KEY_PART}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Nullable</td>
        <td class="details-property-value">{{COLUMN_NULLABLE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Default Value</td>
        <td class="details-property-value">{{COLUMN_DEFAULTVALUE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Auto Increment</td>
        <td class="details-property-value">{{COLUMN_AUTO_INC}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Character Set</td>
        <td class="details-property-value">{{COLUMN_CHARSET}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Collation Name</td>
        <td class="details-property-value">{{COLUMN_COLLATION}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Comment</td>
        <td class="details-property-value">{{COLUMN_COMMENT}}</td>
      </tr>
    </table>
    
    <div class="subitem_header">Data Type Details</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
       <colgroup>
          <col width="20%" />
          <col width="*" />
       </colgroup>
      <tr>
        <td class="details-property-name">Datatype</td>
        <td class="details-property-value">{{COLUMN_DATATYPE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Is User Type</td>
        <td class="details-property-value">{{COLUMN_IS_USERTYPE}}</td>
      </tr>
    </table>
    </div>
    <br />
{{/COLUMNS}}

{{#INDICES}}
  <a name="Index_{{INDEX_ID}}" />
    <div class="table_header">Index {{INDEX_NAME}}</div>
  </a>
 <div class="table_body">
   <div class="subitem_header">General Info</div>
   <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
       <colgroup>
          <col width="25%" />
          <col width="*" />
       </colgroup>
      <tr>
        <td class="details-property-name">Schema Name</td>
        <td class="details-property-value">{{SCHEMA_NAME}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Table Name</td>
        <td class="details-property-value">{{TABLE_NAME}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Is Primary Key</td>
        <td class="details-property-value">{{INDEX_PRIMARY}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Is Unique</td>
        <td class="details-property-value">{{INDEX_UNIQUE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Type</td>
        <td class="details-property-value">{{INDEX_TYPE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Kind</td>
        <td class="details-property-value">{{INDEX_KIND}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Key Block Size</td>
        <td class="details-property-value">{{INDEX_KEY_BLOCK_SIZE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Comment</td>
        <td class="details-property-value">{{INDEX_COMMENT}}</td>
      </tr>
    </table>
    
    <div class="subitem_header">Columns</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
      <tr>
        <td class="subitem_table_head">Column Name</td>
        <td class="subitem_table_head">Column Order</td>
        <td class="subitem_table_head">Comment</td>
      </tr>
  {{#INDEX_COLUMNS}}
      <tr>
        <td class="subitem_table_field">{{INDEX_COLUMN_NAME}}</td>
        <td class="subitem_table_field">{{INDEX_COLUMN_ORDER}}</td>
        <td class="subitem_table_field">{{INDEX_COLUMN_COMMENT}}</td>
      </tr>
  {{/INDEX_COLUMNS}}
    </table>
  </div>
  <br />
{{/INDICES}}

{{#FOREIGN_KEYS}}
  <a name="FK_{{FOREIGN_KEY_ID}}" />
    <div class="table_header">Foreign Key {{REL_NAME}}</div>
  </a>
  <div class="table_body">
    <div class="subitem_header">General Info</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
       <colgroup>
          <col width="25%" />
          <col width="*" />
       </colgroup>
      <tr>
        <td class="details-property-name">Parent Table</td>
        <td class="details-property-value">{{REL_PARENTTABLE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Child Table</td>
        <td class="details-property-value">{{REL_CHILDTABLE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Cardinality</td>
        <td class="details-property-value">{{REL_CARD}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Delete Rule</td>
        <td class="details-property-value">{{FK_DELETE_RULE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Update Rule</td>
        <td class="details-property-value">{{FK_UPDATE_RULE}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Is Mandatory</td>
        <td class="details-property-value">{{FK_MANDATORY}}</td>
      </tr>
    </table>
    </div>
  <br />
{{/FOREIGN_KEYS}}

{{#TRIGGERS}}
  <a name="Trigger_{{TRIGGER_ID}}" />
    <div class="table_header">Trigger {{TRIGGER_NAME}}</div>
  </a>
  <div class="table_body">
    <div class="subitem_header">General Info</div>
    <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
       <colgroup>
          <col width="25%" />
          <col width="*" />
       </colgroup>
      <tr>
        <td class="details-property-name">Enabled</td>
        <td class="details-property-value">{{TRIGGER_ENABLED}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Definer</td>
        <td class="details-property-value">{{TRIGGER_DEFINER}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Condition</td>
        <td class="details-property-value">{{TRIGGER_CONDITION}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Event</td>
        <td class="details-property-value">{{TRIGGER_EVENT}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Order</td>
        <td class="details-property-value">{{TRIGGER_ORDER}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Orientation</td>
        <td class="details-property-value">{{TRIGGER_ORIENTATION}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Timing</td>
        <td class="details-property-value">{{TRIGGER_TIMING}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Reference new</td>
        <td class="details-property-value">{{TRIGGER_REFERENCE_NEW_TABLE}}.{{TRIGGER_REFERENCE_NEW_ROW}}</td>
      </tr>
      <tr>
        <td class="details-property-name">Reference old</td>
        <td class="details-property-value">{{TRIGGER_REFERENCE_OLD_TABLE}}.{{TRIGGER_REFERENCE_OLD_ROW}}</td>
      </tr>
    </table>
		{{#DDL_LISTING}}
		  <div class="subitem_header">DDL script</div>
		  <div class="ddl_field"><pre>{{DDL_SCRIPT}}</pre></div>
		{{/DDL_LISTING}}
    </div>
  <br />
{{/TRIGGERS}}

{{/SCHEMATA}}
</body>
</html>
