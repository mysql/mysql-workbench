<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>{{TITLE}} - Table Details</title>
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>
  
<body class="tbl_detail_page">
{{#SCHEMATA}}

{{#VIEWS}}
  <a name="View_{{VIEW_ID}}">
    <div class="table_header">View {{VIEW_NAME}} <div class="small_text">({{VIEW_NUMBER}}/{{VIEW_COUNT}})</div></div>
  </a>
  <div class="table_body">
	
	<!-- View properties -->
	<div class="subitem_header">View Properties</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
     <colgroup>
        <col width="20%" />
        <col width="*" />
     </colgroup>
    <tr>
      <td class="details-property-name">Columns</td>
      <td class="details-property-value">{{VIEW_COLUMNS}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Access Type</td>
      <td class="details-property-value">{{VIEW_READ_ONLY}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Has Check Condition</td>
      <td class="details-property-value">{{VIEW_WITH_CHECK}}</td>
    </tr>
  </table>

{{#VIEW_COMMENT_LISTING}}
  <div class="subitem_header">Table Comments</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="" width="100%">
    <tr>
      <td class="subitem_table_field">{{TABLE_COMMENT}}</td>
    </tr>
  </table>
{{/VIEW_COMMENT_LISTING}}

{{#DDL_LISTING}}
  <div class="subitem_header">DDL script</div>
  <div class="ddl_field"><pre>{{DDL_SCRIPT}}</pre></div>
{{/DDL_LISTING}}
  </div>
{{/VIEWS}}

{{/SCHEMATA}}
</body>
</html>
