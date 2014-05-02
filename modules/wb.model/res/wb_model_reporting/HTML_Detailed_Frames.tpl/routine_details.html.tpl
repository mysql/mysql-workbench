<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>{{TITLE}} - Table Details</title>
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>
  
<body class="tbl_detail_page">
{{#SCHEMATA}}

{{#ROUTINES}}
  <a name="Routine_{{ROUTINE_ID}}">
    <div class="table_header">Routine {{ROUTINE_NAME}} <span class="small_text">({{ROUTINE_NUMBER}}/{{ROUTINE_COUNT}})</span></div>
  </a>
  <div class="table_body">
	
	<!-- Routine properties -->
	<div class="subitem_header">Routine Properties</div>
  <table class="subitems_table" border="0" cellpadding="2" cellspacing="0" width="100%">
     <colgroup>
        <col width="20%" />
        <col width="*" />
     </colgroup>
    <tr>
      <td class="details-property-name">Schema</td>
      <td class="details-property-value">{{SCHEMA_NAME}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Type</td>
      <td class="details-property-value">{{ROUTINE_TYPE}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Parameters</td>
      <td class="details-property-value">{{ROUTINE_PARAMETER_COUNT}}</td>
    </tr>
{{#ROUTINE_PARAMETERS}}
    <tr>
      <td class="details-sub-property-name">{{ROUTINE_PARAMETER_NAME}}</td>
      <td class="details-sub-property-value">{{ROUTINE_PARAMETER_DATA_TYPE}} <b>{{ROUTINE_PARAMETER_TYPE}}</b></td>
    </tr>
{{/ROUTINE_PARAMETERS}}
    <tr>
      <td class="details-property-name">Return Data Type</td>
      <td class="details-property-value">{{ROUTINE_RETURN_TYPE}}</td>
    </tr>
    <tr>
      <td class="details-property-name">Security</td>
      <td class="details-property-value">{{ROUTINE_SECURITY}}</td>
    </tr>
  </table>

{{#DDL_LISTING}}
  <div class="subitem_header">DDL script</div>
  <div class="ddl_field"><pre>{{DDL_SCRIPT}}</pre></div>
{{/DDL_LISTING}}
  </div>
{{/ROUTINES}}

{{/SCHEMATA}}
</body>
</html>
