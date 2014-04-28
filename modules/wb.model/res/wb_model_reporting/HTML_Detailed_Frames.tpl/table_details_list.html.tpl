<html xmlns="http://www.w3.org/1999/xhtml">
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"  "http://www.w3.org/TR/html4/loose.dtd">
<head>
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>{{TITLE:h}} - Overview</title>
  <link rel="stylesheet" type="text/css" media="screen" href="{{STYLE_NAME}}.css">
</head>

<body class="ov_overview_page">
<div class="ov_main">
  <div class="overview-heading"><a href="overview.html" target="content">Schema Overview</a></div>

{{#SCHEMATA}}
  <div class="ov_section_link">
    <a name="Schema_{{SCHEMA_ID}}" />
    <a href="table_details.html#Schema_{{SCHEMA_ID}}" target="content">Schema {{SCHEMA_NAME}} <img src="images/next.png"/></a><br />
    <a href="overview_list.html#Schema_{{SCHEMA_ID}}" target="overview" class="small_text"><img src="images/back.png"/> Back to overview</a>
  </div>
  <div class="ov_object_link">
    <div class="ov_section_subitem">Columns ({{TOTAL_COLUMN_COUNT}})</div>
{{#COLUMNS}}
    <a href="table_element_details.html#Column_{{COLUMN_ID}}" target="content"><span class="ov_section_qualifier">{{TABLE_NAME}}</span>.{{COLUMN_NAME}}</a>
{{/COLUMNS}}
    
    <div class="ov_section_subitem">Indices ({{TOTAL_INDEX_COUNT}})</div>
{{#INDICES}}
    <a href="table_element_details.html#Index_{{INDEX_ID}}" target="content"><span class="ov_section_qualifier">{{TABLE_NAME}}</span>.{{INDEX_NAME}}</a>
{{/INDICES}}
    
    <div class="ov_section_subitem">Foreign Keys ({{TOTAL_FOREIGN_KEY_COUNT}})</div>
{{#FOREIGN_KEYS}}
    <a href="table_element_details.html#FK_{{FOREIGN_KEY_ID}}" target="content"><span class="ov_section_qualifier">{{TABLE_NAME}}</span>.{{REL_NAME}}</a>
{{/FOREIGN_KEYS}}

    <div class="ov_section_subitem">Triggers ({{TOTAL_TRIGGER_COUNT}})</div>
{{#TRIGGERS}}
    <a href="table_element_details.html#Trigger_{{TRIGGER_ID}}" target="content"><span class="ov_section_qualifier">{{TABLE_NAME}}</span>.{{TRIGGER_NAME}}</a>
{{/TRIGGERS}}
    
  </div>
{{/SCHEMATA}}

</div>
</body>

</html>
