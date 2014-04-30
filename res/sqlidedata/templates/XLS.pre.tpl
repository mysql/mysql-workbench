<?xml version="1.0" encoding="UTF-8"?>
<?mso-application progid="Excel.Sheet"?>
<Workbook xmlns="urn:schemas-microsoft-com:office:spreadsheet"
xmlns:o="urn:schemas-microsoft-com:office:office"
xmlns:x="urn:schemas-microsoft-com:office:excel"
xmlns:ss="urn:schemas-microsoft-com:office:spreadsheet"
xmlns:html="http://www.w3.org/TR/REC-html40">
<Worksheet ss:Name="Table1">
{{INDENT}}<Table>{{#STRING_COLUMN}}
{{INDENT}}{{INDENT}}<Column ss:Index="{{STRING_COLUMN_INDEX}}" ss:AutoFitWidth="0" ss:Width="110"/>{{/STRING_COLUMN}}
{{INDENT}}{{INDENT}}<Row>{{#COLUMN}}
{{INDENT}}{{INDENT}}{{INDENT}}<Cell><Data ss:Type="String">{{COLUMN_NAME}}</Data></Cell>{{/COLUMN}}
{{INDENT}}{{INDENT}}</Row>
