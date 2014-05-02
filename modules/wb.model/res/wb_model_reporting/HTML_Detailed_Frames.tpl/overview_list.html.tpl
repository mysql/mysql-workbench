<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>{{TITLE:h}} - Overview</title>
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>

<body class="overview_list">
<div class="ov_main">
  <div class="overview-heading"><a href="overview.html" target="content">Schema Overview</a></div>

{{#SCHEMATA}}
  <div class="ov_section_link">
    <a name="Schema_{{SCHEMA_ID}}" />
    <a href="table_details.html#Schema_{{SCHEMA_ID}}" target="content">Schema {{SCHEMA_NAME}} <img src="images/next.png"/></a><br />
    <a href="table_details_list.html#Schema_{{SCHEMA_ID}}" target="overview" class="small_text">Columns, Indices and Triggers <img src="images/next.png"/></a>
  </div>

  <div class="ov_object_link">
    <div class="ov_section_subitem">Schema Tables ({{TABLE_COUNT}})</div>
{{#TABLES}}
    <a href="table_details.html#Table_{{TABLE_ID}}" target="content">{{TABLE_NAME}}</a>
{{/TABLES}}
  </div>
  
  <div class="ov_object_link">
    <div class="ov_section_subitem">Schema Views ({{VIEW_COUNT}})</div>
{{#VIEWS}}
    <a href="view_details.html#View_{{VIEW_ID}}" target="content">{{VIEW_NAME}}</a>
{{/VIEWS}}
  </div>

  <div class="ov_object_link">
    <div class="ov_section_subitem">Routines ({{ROUTINE_COUNT}})</div>
{{#ROUTINES}}
    <a href="routine_details.html#Routine_{{ROUTINE_ID}}" target="content">{{ROUTINE_NAME}}</a>
{{/ROUTINES}}
  </div>

{{/SCHEMATA}}

</div>
</body>

</html>
