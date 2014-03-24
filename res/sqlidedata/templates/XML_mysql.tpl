{{#ROW}}
{{INDENT}}<row>{{#FIELD}}
{{INDENT}}{{INDENT}}{{#FIELD_is_null}}<field name="{{FIELD_NAME:xml_escape}}" xsi:nil="true" />{{/FIELD_is_null}}{{#FIELD_is_not_null}}<field name="{{FIELD_NAME:xml_escape}}">{{FIELD_VALUE:xml_escape}}</field>{{/FIELD_is_not_null}}{{/FIELD}}
{{INDENT}}</row>{{/ROW}}
