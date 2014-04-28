/* General settings */
body {
	color: #000000;
	font-family : Trebuchet MS, Arial, Verdana, sans-serif;
	background : #FFFFFF;
	margin-bottom : 0px;
	margin-left : 0px;
	margin-right : 0px;
	margin-top : 0px;
}

img {
	border: none;
}

a {
	color: white;
	text-decoration: none;
}

a:hover {
	color: white;
	text-decoration: underline;
}

a:focus {
	border: none;
	outline: none;
}

.report_na_entry {
	color: #AAA;
}

.top-title {
	font-family: Tahoma, Verdana, sans-serif;
	font-size: 200%;
	font-weight: bold;
	vertical-align: middle;
	background: url(images/title-background.png) no-repeat right;
  text-aling: center;
  width: 100%;
}
	
/* Sections in navigation bar */
.overview-heading {
	padding-top: 8px;
	padding-bottom: 8px;
	color: white;
}

.overview-heading a {
	display: block;
	width: 100%;
}

.ov_section_link {
	background: #4D9100;
	color: white;
	font-weight : normal;
	text-align : center;
	width : 100%;
	display : block;
	padding-top: 12px;
	padding-bottom: 12px;
	border-bottom: 1px solid #B7B7B7;
}

.ov_section_link a {
	color: white;
	font-weight : bold;
	text-align : center;
	text-decoration: none;
	display : inline;
	width: 100%;
}

.ov_section_link a:hover {
	font-weight: bold;
	text-decoration: underline;
	color: white;
}

.ov_section_qualifier {
	color: #3D8100;
}

.ov_section_subitem {
	background: #5DA110;
	font-weight: normal;
	text-align: center;
	width: 100%;
	padding-top: 2px;
	padding-bottom: 4px;
	display : block;
	height: 14px;
}

/* Object links in navigation bar */
.ov_object_link {
	font-size: 75%;
	font-weight: normal;
	text-align: left;
	text-indent: 15px;
	width: 100%;
}

.ov_object_link a {
	color: black;
	background: #E7F7D4;
	text-decoration : none;
	display : block;
	border-top: 1px solid #D7E7C4; 
	width: 100%;
}

.ov_object_link a:hover {
	background: white;
}

.ov_object_link a:focus {
	background: white;
	outline: none;
}

/* Overview on content page */
.overview {
	margin-left: auto;
	margin-right: auto;
	padding: 50px;
	background: #F0F0F0;
	border: 1px solid #CCC;
}
	
.overview-head {
	font-family: Tahoma, sans-serif;
	text-align: center;
	text-decoration: none;
	margin-top: 50px;
}

.property-name {
	font-weight: bold;
}

.property-value {
	color: #4D9100;
}

/* Table details on content page */
.tbl_detail_page {
	margin: 10px;
}

.schema_header {
	background: #FB9B21;
	font-size : 12pt;
	font-weight : bold;
	padding-left : 10px;
	padding-bottom : 5px;
	padding-top : 5px;
	border: 1px solid #888888;
	display : block;
}

/* Full details on content page */
.full_detail_page {
	padding-left: 10px;
	padding-right: 10px;
}

/* Shared formatting table details and full details */
.table_header {
	background: #DB7B01;
	font-size : 10pt;
	font-weight : bold;
	padding-left : 10px;
	padding-bottom : 5px;
	padding-top : 5px;
	border: 1px solid #BBBBBB;
	display : block;
}

.table_body {
	background: #EEEEEE;
	padding: 15px;
	border-left: 1px solid #BBBBBB;
	border-bottom: 1px solid #BBBBBB;
	border-right: 1px solid #BBBBBB;
  margin-bottom:10px;
}

.subitem_header {
	color: black;
	font-size : 75%;
	font-weight : bold;
	padding-bottom : 5px;
	padding-top : 5px;
}

.subitems_table {
	border:1px solid #CCCCCC;
	margin-bottom: 15px;
}

.subitem_table_head {
	color: white;
	background: #DB4801;
	font-size : 90%;
	font-weight : bold;
	text-align : left;
	border: 0pt solid #000000;
	padding-bottom : 3px;
	padding-left : 10px;
	padding-right : 2px;
	padding-top : 3px;
}

.details-property-name {
	color: white;
	background: #DB4801;
	font-size: 75%;
	font-weight: bold;
	text-align: left;
	border: 0pt solid #000000;
	padding-bottom : 3px;
	padding-left: 10px;
	padding-right: 2px;
	padding-top: 3px;
}

.details-property-value {
	color: #333333;
	font-size: 75%;
	font-weight: normal;
	text-align: left;
	border: 0pt solid #000000;
	padding-bottom: 3px;
	padding-left: 10px;
	padding-right: 2px;
	padding-top: 3px;
	vertical-align: top;
	background: white;
}

.details-sub-property-name {
	color: white;
	background: #E5520B;
	font-size: 75%;
	font-weight: normal;
	text-align: left;
	border: none;
	padding: 3px 2px 3px 30px;
}

.details-sub-property-value {
	color: #333333;
	background: #F0F0F0;
	font-size: 75%;
	font-weight: normal;
	text-align: left;
	border: none;
	padding: 3px 2px 3px 30px;
	vertical-align: top;
}

.properties-table-head {
	font-size: 80%;
	background: #F7DFD4;
	font-weight: bold;
}

.subitem_table_field {
	color: #333333;
	font-size: 75%;
	font-weight: normal;
	text-align: left;
	border: 0pt solid #000000;
	padding-bottom: 3px;
	padding-left: 10px;
	padding-right: 2px;
	padding-top: 3px;
	vertical-align: top;
	background:white;
}

.ddl_field {
	color: #333333;
	font-size:85%;
	font-weight: normal;
	text-align: left;
	padding: 5px 5px 0px 5px;
	vertical-align: top;
	background: white;
	border:1px solid #CCCCCC;
}

.ddl_field pre {
	overflow: auto;
}

.small_text {
	font-size: 75%;
}

.ov_overview_page {
	background: white;
}

.ov_main {
	background: #007791;
	color: white;
	font-weight: bold;
	display : block;
	text-align : center;
	padding-left : 0px;
	padding-right : 0px;
}

/* Syntax highlighting styles ------------------------------------ */
.syntax_default {
	color: black;
	font-weight: normal;
}

.syntax_comment {
	color: #097BF7;
	font-weight: normal;
}

.syntax_comment_line {
	color: #097BF7;
	font-weight: normal;
}

.syntax_variable {
	color: #378EA5;
	font-weight: normal;
}

.syntax_system_variable {
	color: #378EA5;
	font-weight: normal;
}

.syntax_known_system_variable {
	color: #3A37A5;
	font-weight: normal;
}

.syntax_number {
	color: #7F7F00;
	font-weight: normal;
}

.syntax_major_keyword {
	color: black;
	font-weight: bold;
}

.syntax_keyword {
	color: #007F00;
	font-weight: bold;
}

.syntax_database_object {
	color: red;
	font-weight: normal;
}

.syntax_procedure_keyword {
	color: #56007F;
	font-weight: bold;
}

.syntax_string {
	color: #FFAA3E;
	font-weight: normal;
}

.syntax_single_quoted_string {
	color: #FFAA3E;
	font-weight: normal;
}

.syntax_double_quoted_string {
	color: #274A6D;
	font-weight: normal;
}

.syntax_operator {
	color: black;
	font-weight: bold;
}

.syntax_function {
	color: #903600;
	font-weight: normal;
}

.syntax_identifier {
	color: black;
	font-weight: normal;
}

.syntax_quoted_identifier {
	color: #274A6D;
	font-weight: normal;
}

.syntax_user1 {
	color: #808080;
	font-weight: normal;
}

.syntax_user2 {
	color: #808080;
	font-weight: normal;
}

.syntax_user3 {
	color: #E0E0E0;
	font-weight: normal;
}

.syntax_hidden_command {
	color: #097BF7;
	background: #F0F0F0;
	font-weight: normal;
}

