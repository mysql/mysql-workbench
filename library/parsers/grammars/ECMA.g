/*
 * This ANTLR 3 LL(*) grammar is based on Ecma-262 3rd edition (JavaScript 1.5, JScript 5.5). 
 * The grammar was kept as close as possible to the grammar in the "A Grammar Summary" section of Ecma-262.
 *
 */

grammar ECMA;

options
{
  output = AST; // Actually, we don't need the AST for syntax checks but we use the generated parser also to determine
                // statement ranges, so we need the tree for token positions.
  language = C;
  k = 2;
  ASTLabelType = pANTLR3_BASE_TREE;
}

tokens
{
	// Reserved words.
	NULL_SYMBOL			= 'null';
	TRUE_SYMBOL			= 'true';
	FALSE_SYMBOL		= 'false';

	// Keywords
	BREAK_SYMBOL		= 'break';
	CASE_SYMBOL			= 'case';
	CATCH_SYMBOL		= 'catch';
	CONTINUE_SYMBOL		= 'continue';
	DEFAULT_SYMBOL		= 'default';
	DELETE_SYMBOL		= 'delete';
	DO_SYMBOL			= 'do';
	ELSE_SYMBOL			= 'else';
	FINALLY_SYMBOL		= 'finally';
	FOR_SYMBOL			= 'for';
	FUNCTION_SYMBOL		= 'function';
	IF_SYMBOL			= 'if';
	IN_SYMBOL			= 'in';
	INSTANCEOF_SYMBOL	= 'instanceof';
	NEW_SYMBOL			= 'new';
	RETURN_SYMBOL		= 'return';
	SWITCH_SYMBOL		= 'switch';
	THIS_SYMBOL			= 'this';
	THROW_SYMBOL		= 'throw';
	TRY_SYMBOL			= 'try';
	TYPEOF_SYMBOL		= 'typeof';
	VAR_SYMBOL			= 'var';
	VOID_SYMBOL			= 'void';
	WHILE_SYMBOL		= 'while';
	WITH_SYMBOL			= 'with';

	// Future reserved words.
	ABSTRACT_SYMBOL		= 'abstract';
	BOOLEAN_SYMBOL		= 'boolean';
	BYTE_SYMBOL			= 'byte';
	CHAR_SYMBOL			= 'char';
	CLASS_SYMBOL		= 'class';
	CONST_SYMBOL		= 'const';
	DEBUGGER_SYMBOL		= 'debugger';
	DOUBLE_SYMBOL		= 'double';
	ENUM_SYMBOL			= 'enum';
	EXPORT_SYMBOL		= 'export';
	EXTENDS_SYMBOL		= 'extends';
	FINAL_SYMBOL		= 'final';
	FLOAT_SYMBOL		= 'float';
	GOTO_SYMBOL			= 'goto';
	IMPLEMENTS_SYMBOL	= 'implements';
	IMPORT_SYMBOL		= 'import';
	INT_SYMBOL			= 'int';
	INTERFACE_SYMBOL	= 'interface';
	LONG_SYMBOL			= 'long';
	NATIVE_SYMBOL		= 'native';
	PACKAGE_SYMBOL		= 'package';
	PRIVATE_SYMBOL		= 'private';
	PROTECTED_SYMBOL	= 'protected';
	PUBLIC_SYMBOL		= 'public';
	SHORT_SYMBOL		= 'short';
	STATIC_SYMBOL		= 'static';
	SUPER_SYMBOL		= 'super';
	SYNCHRONIZED_SYMBOL	= 'synchronized';
	THROWS_SYMBOL		= 'throws';
	TRANSIENT_SYMBOL	= 'transient';
	VOLATILE_SYMBOL		= 'volatile';

	// Punctuators
	LEFT_BRACE_SYMBOL			= '{';
	RIGHT_BRACE_SYMBOL			= '}';
	LEFT_PAR_SYMBOL				= '(';
	RIGHT_PAR_SYMBOL			= ')';
	LEFT_BRACKET_SYMBOL			= '[';
	RIGHT_BRACKET_SYMBOL		= ']';
	DOT_SYMBOL					= '.';
	SEMICOLON_SYMBOL			= ';';
	COMMA_SYMBOL				= ',';
	LESS_THAN_OPERATOR			= '<';
	GREATER_THAN_OPERATOR		= '>';
	LESS_OR_EQUAL_OPERATOR		= '<=';
	GREATER_OR_EQUAL_OPERATOR	= '>=';
	EQUAL_OPERATOR				= '==';
	NOT_EQUAL_OPERATOR			= '!=';
	SAME_OPERATOR				= '===';
	NOT_SAME_OPERATOR			= '!==';
	PLUS_OPERATOR				= '+';
	MINUS_OPERATOR				= '-';
	MULT_OPERATOR				= '*';
	MODULO_OPERATOR				= '%';
	INCREMENT_OPERATOR			= '++';
	DECREMENT_OPERATOR			= '--';
	SHIFT_LEFT_OPERATOR			= '<<';
	SHIFT_RIGHT_OPERATOR		= '>>';
	UNSIGNED_SHIFT_RIGHT_OPERATOR	= '>>>';
	BITWISE_AND_OPERATOR		= '&';
	BITWISE_OR_OPERATOR			= '|';
	BITWISE_XOR_OPERATOR		= '^';
	LOGICAL_NOT_OPERATOR		= '!';
	BITWISE_NOT_OPERATOR		= '~';
	LOGICAL_AND_OPERATOR		= '&&';
	LOGICAL_OR_OPERATOR			= '||';
	QUESTION_MARK_SYMBOL		= '?';
	COLON_SYMBOL				= ':';
	ASSIGN_OPERATOR				= '=';
	PLUS_ASSIGN_OPERATOR		= '+=';
	MINUS_ASSIGN_OPERATOR		= '-=';
	MULT_ASSIGN_OPERATOR		= '*=';
	MOD_ASSIGN_OPERATOR			= '%=';
	SHIFT_LEFT_ASSIGN_OPERATOR	= '<<=';
	SHIFT_RIGHT_ASSIGN_OPERATOR	= '>>=';
	UNSIGNED_SHIFT_RIGHT_ASSIGN_OPERATOR	= '>>>=';
	BITWISE_AND_ASSIGN_OPERATOR	= '&=';
	BITWISE_OR_ASSIGN_OPERATOR	= '|=';
	BITWISE_XOR_ASSIGN_OPERATOR	= '^=';
	DIV_OPERATOR				= '/';
	DIV_ASSIGN_OPERATOR			= '/=';
	
	// Imaginary tokens.
	ARGS_TOKEN;
	ARRAY_TOKEN;
	BLOCK_TOKEN;
	BYFIELD_TOKEN;
	BYINDEX_TOKEN;
	CALL_TOKEN;
	CEXPR_TOKEN;
	EXPR_TOKEN;
	FORITER_TOKEN;
	FORSTEP_TOKEN;
	ITEM_TOKEN;
	LABELLED_TOKEN;
	NAMED_VALUE_TOKEN;
	NEG_TOKEN;
	OBJECT_TOKEN;
	PAREXPR_TOKEN;
	PDEC_TOKEN;
	PINC_TOKEN;
	POS_TOKEN;
}

@lexer::header {

#ifndef _WIN32
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #pragma GCC diagnostic ignored "-Wparentheses"
  #ifdef __APPLE__
    // Comparison of unsigned expression >= 0 is always true.
    #pragma GCC diagnostic ignored "-Wtautological-compare"
  #else
    #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6 )
    #pragma GCC diagnostic ignored "-Wtype-limits"
  #endif
#endif
#else
  #pragma warning(disable:4296) // Condition is always true.
#endif

}

@lexer::postinclude {
#ifdef __cplusplus
  extern "C" { 
#endif

  // Custom error reporting function.
  void onECMAParseError(struct ANTLR3_BASE_RECOGNIZER_struct *recognizer, pANTLR3_UINT8 *tokenNames); 
  
#ifdef __cplusplus
  };
#endif
}

@lexer::members
{
static pANTLR3_COMMON_TOKEN last = NULL;

ANTLR3_BOOLEAN regex_enabled(pECMALexer ctx)
{
  if (last == NULL)
    return ANTLR3_TRUE;
    
  switch (last->getType(last))
  {
    case Identifier:
    case NULL_SYMBOL:
    case TRUE_SYMBOL:
    case FALSE_SYMBOL:
    case THIS_SYMBOL:
    case OctalIntegerLiteral:
    case DecimalLiteral:
    case HexIntegerLiteral:
    case StringLiteral:
    case RIGHT_BRACKET_SYMBOL:
    case RIGHT_PAR_SYMBOL:
      return ANTLR3_FALSE;
    default:
      return ANTLR3_TRUE;
  }
}

// ECMA identifier letters according to http://www.ecma-international.org/ecma-262/5.1/#sec-7.6.
static ANTLR3_BOOLEAN isUnicodeLetter(int ch)
{
  return (ch >= 0x0041 && ch <= 0x005A)
    || (ch >= 0x0061 && ch <= 0x007A)
    || (ch == 0x00AA)
    || (ch == 0x00B5)
    || (ch == 0x00BA)
    || (ch >= 0x00C0 && ch <= 0x00D6)
    || (ch >= 0x00D8 && ch <= 0x00F6)
    || (ch >= 0x00F8 && ch <= 0x021F)
    || (ch >= 0x0222 && ch <= 0x0233)
    || (ch >= 0x0250 && ch <= 0x02AD)
    || (ch >= 0x02B0 && ch <= 0x02B8)
    || (ch >= 0x02BB && ch <= 0x02C1)
    || (ch >= 0x02D0 && ch <= 0x02D1)
    || (ch >= 0x02E0 && ch <= 0x02E4)
    || (ch == 0x02EE)
    || (ch == 0x037A)
    || (ch == 0x0386)
    || (ch >= 0x0388 && ch <= 0x038A)
    || (ch == 0x038C)
    || (ch >= 0x038E && ch <= 0x03A1)
    || (ch >= 0x03A3 && ch <= 0x03CE)
    || (ch >= 0x03D0 && ch <= 0x03D7)
    || (ch >= 0x03DA && ch <= 0x03F3)
    || (ch >= 0x0400 && ch <= 0x0481)
    || (ch >= 0x048C && ch <= 0x04C4)
    || (ch >= 0x04C7 && ch <= 0x04C8)
    || (ch >= 0x04CB && ch <= 0x04CC)
    || (ch >= 0x04D0 && ch <= 0x04F5)
    || (ch >= 0x04F8 && ch <= 0x04F9)
    || (ch >= 0x0531 && ch <= 0x0556)
    || (ch == 0x0559)
    || (ch >= 0x0561 && ch <= 0x0587)
    || (ch >= 0x05D0 && ch <= 0x05EA)
    || (ch >= 0x05F0 && ch <= 0x05F2)
    || (ch >= 0x0621 && ch <= 0x063A)
    || (ch >= 0x0640 && ch <= 0x064A)
    || (ch >= 0x0671 && ch <= 0x06D3)
    || (ch == 0x06D5)
    || (ch >= 0x06E5 && ch <= 0x06E6)
    || (ch >= 0x06FA && ch <= 0x06FC)
    || (ch >= 0x0710)
    || (ch >= 0x0712 && ch <= 0x072C)
    || (ch >= 0x0780 && ch <= 0x07A5)
    || (ch >= 0x0905 && ch <= 0x0939)
    || (ch >= 0x093D)
    || (ch == 0x0950)
    || (ch >= 0x0958 && ch <= 0x0961)
    || (ch >= 0x0985 && ch <= 0x098C)
    || (ch >= 0x098F && ch <= 0x0990)
    || (ch >= 0x0993 && ch <= 0x09A8)
    || (ch >= 0x09AA && ch <= 0x09B0)
    || (ch == 0x09B2)
    || (ch >= 0x09B6 && ch <= 0x09B9)
    || (ch >= 0x09DC && ch <= 0x09DD)
    || (ch >= 0x09DF && ch <= 0x09E1)
    || (ch >= 0x09F0 && ch <= 0x09F1)
    || (ch >= 0x0A05 && ch <= 0x0A0A)
    || (ch >= 0x0A0F && ch <= 0x0A10)
    || (ch >= 0x0A13 && ch <= 0x0A28)
    || (ch >= 0x0A2A && ch <= 0x0A30)
    || (ch >= 0x0A32 && ch <= 0x0A33)
    || (ch >= 0x0A35 && ch <= 0x0A36)
    || (ch >= 0x0A38 && ch <= 0x0A39)
    || (ch >= 0x0A59 && ch <= 0x0A5C)
    || (ch == 0x0A5E)
    || (ch >= 0x0A72 && ch <= 0x0A74)
    || (ch >= 0x0A85 && ch <= 0x0A8B)
    || (ch == 0x0A8D)
    || (ch >= 0x0A8F && ch <= 0x0A91)
    || (ch >= 0x0A93 && ch <= 0x0AA8)
    || (ch >= 0x0AAA && ch <= 0x0AB0)
    || (ch >= 0x0AB2 && ch <= 0x0AB3)
    || (ch >= 0x0AB5 && ch <= 0x0AB9)
    || (ch == 0x0ABD)
    || (ch == 0x0AD0)
    || (ch == 0x0AE0)
    || (ch >= 0x0B05 && ch <= 0x0B0C)
    || (ch >= 0x0B0F && ch <= 0x0B10)
    || (ch >= 0x0B13 && ch <= 0x0B28)
    || (ch >= 0x0B2A && ch <= 0x0B30)
    || (ch >= 0x0B32 && ch <= 0x0B33)
    || (ch >= 0x0B36 && ch <= 0x0B39)
    || (ch == 0x0B3D)
    || (ch >= 0x0B5C && ch <= 0x0B5D)
    || (ch >= 0x0B5F && ch <= 0x0B61)
    || (ch >= 0x0B85 && ch <= 0x0B8A)
    || (ch >= 0x0B8E && ch <= 0x0B90)
    || (ch >= 0x0B92 && ch <= 0x0B95)
    || (ch >= 0x0B99 && ch <= 0x0B9A)
    || (ch == 0x0B9C)
    || (ch >= 0x0B9E && ch <= 0x0B9F)
    || (ch >= 0x0BA3 && ch <= 0x0BA4)
    || (ch >= 0x0BA8 && ch <= 0x0BAA)
    || (ch >= 0x0BAE && ch <= 0x0BB5)
    || (ch >= 0x0BB7 && ch <= 0x0BB9)
    || (ch >= 0x0C05 && ch <= 0x0C0C)
    || (ch >= 0x0C0E && ch <= 0x0C10)
    || (ch >= 0x0C12 && ch <= 0x0C28)
    || (ch >= 0x0C2A && ch <= 0x0C33)
    || (ch >= 0x0C35 && ch <= 0x0C39)
    || (ch >= 0x0C60 && ch <= 0x0C61)
    || (ch >= 0x0C85 && ch <= 0x0C8C)
    || (ch >= 0x0C8E && ch <= 0x0C90)
    || (ch >= 0x0C92 && ch <= 0x0CA8)
    || (ch >= 0x0CAA && ch <= 0x0CB3)
    || (ch >= 0x0CB5 && ch <= 0x0CB9)
    || (ch == 0x0CDE)
    || (ch >= 0x0CE0 && ch <= 0x0CE1)
    || (ch >= 0x0D05 && ch <= 0x0D0C)
    || (ch >= 0x0D0E && ch <= 0x0D10)
    || (ch >= 0x0D12 && ch <= 0x0D28)
    || (ch >= 0x0D2A && ch <= 0x0D39)
    || (ch >= 0x0D60 && ch <= 0x0D61)
    || (ch >= 0x0D85 && ch <= 0x0D96)
    || (ch >= 0x0D9A && ch <= 0x0DB1)
    || (ch >= 0x0DB3 && ch <= 0x0DBB)
    || (ch == 0x0DBD)
    || (ch >= 0x0DC0 && ch <= 0x0DC6)
    || (ch >= 0x0E01 && ch <= 0x0E30)
    || (ch >= 0x0E32 && ch <= 0x0E33)
    || (ch >= 0x0E40 && ch <= 0x0E46)
    || (ch >= 0x0E81 && ch <= 0x0E82)
    || (ch == 0x0E84)
    || (ch >= 0x0E87 && ch <= 0x0E88)
    || (ch == 0x0E8A)
    || (ch == 0x0E8D)
    || (ch >= 0x0E94 && ch <= 0x0E97)
    || (ch >= 0x0E99 && ch <= 0x0E9F)
    || (ch >= 0x0EA1 && ch <= 0x0EA3)
    || (ch == 0x0EA5)
    || (ch >= 0x0EA7)
    || (ch >= 0x0EAA && ch <= 0x0EAB)
    || (ch >= 0x0EAD && ch <= 0x0EB0)
    || (ch >= 0x0EB2 && ch <= 0x0EB3)
    || (ch >= 0x0EBD && ch <= 0x0EC4)
    || (ch == 0x0EC6)
    || (ch >= 0x0EDC && ch <= 0x0EDD)
    || (ch == 0x0F00)
    || (ch >= 0x0F40 && ch <= 0x0F6A)
    || (ch >= 0x0F88 && ch <= 0x0F8B)
    || (ch >= 0x1000 && ch <= 0x1021)
    || (ch >= 0x1023 && ch <= 0x1027)
    || (ch >= 0x1029 && ch <= 0x102A)
    || (ch >= 0x1050 && ch <= 0x1055)
    || (ch >= 0x10A0 && ch <= 0x10C5)
    || (ch >= 0x10D0 && ch <= 0x10F6)
    || (ch >= 0x1100 && ch <= 0x1159)
    || (ch >= 0x115F && ch <= 0x11A2)
    || (ch >= 0x11A8 && ch <= 0x11F9)
    || (ch >= 0x1200 && ch <= 0x1206)
    || (ch >= 0x1208 && ch <= 0x1246)
    || (ch == 0x1248)
    || (ch >= 0x124A && ch <= 0x124D)
    || (ch >= 0x1250 && ch <= 0x1256)
    || (ch == 0x1258)
    || (ch >= 0x125A && ch <= 0x125D)
    || (ch >= 0x1260 && ch <= 0x1286)
    || (ch == 0x1288)
    || (ch >= 0x128A && ch <= 0x128D)
    || (ch >= 0x1290 && ch <= 0x12AE)
    || (ch == 0x12B0)
    || (ch >= 0x12B2 && ch <= 0x12B5)
    || (ch >= 0x12B8 && ch <= 0x12BE)
    || (ch == 0x12C0)
    || (ch >= 0x12C2 && ch <= 0x12C5)
    || (ch >= 0x12C8 && ch <= 0x12CE)
    || (ch >= 0x12D0 && ch <= 0x12D6)
    || (ch >= 0x12D8 && ch <= 0x12EE)
    || (ch >= 0x12F0 && ch <= 0x130E)
    || (ch == 0x1310)
    || (ch >= 0x1312 && ch <= 0x1315)
    || (ch >= 0x1318 && ch <= 0x131E)
    || (ch >= 0x1320 && ch <= 0x1346)
    || (ch >= 0x1348 && ch <= 0x135A)
    || (ch >= 0x13A0 && ch <= 0x13B0)
    || (ch >= 0x13B1 && ch <= 0x13F4)
    || (ch >= 0x1401 && ch <= 0x1676)
    || (ch >= 0x1681 && ch <= 0x169A)
    || (ch >= 0x16A0 && ch <= 0x16EA)
    || (ch >= 0x1780 && ch <= 0x17B3)
    || (ch >= 0x1820 && ch <= 0x1877)
    || (ch >= 0x1880 && ch <= 0x18A8)
    || (ch >= 0x1E00 && ch <= 0x1E9B)
    || (ch >= 0x1EA0 && ch <= 0x1EE0)
    || (ch >= 0x1EE1 && ch <= 0x1EF9)
    || (ch >= 0x1F00 && ch <= 0x1F15)
    || (ch >= 0x1F18 && ch <= 0x1F1D)
    || (ch >= 0x1F20 && ch <= 0x1F39)
    || (ch >= 0x1F3A && ch <= 0x1F45)
    || (ch >= 0x1F48 && ch <= 0x1F4D)
    || (ch >= 0x1F50 && ch <= 0x1F57)
    || (ch == 0x1F59)
    || (ch == 0x1F5B)
    || (ch == 0x1F5D)
    || (ch >= 0x1F5F && ch <= 0x1F7D)
    || (ch >= 0x1F80 && ch <= 0x1FB4)
    || (ch >= 0x1FB6 && ch <= 0x1FBC)
    || (ch == 0x1FBE)
    || (ch >= 0x1FC2 && ch <= 0x1FC4)
    || (ch >= 0x1FC6 && ch <= 0x1FCC)
    || (ch >= 0x1FD0 && ch <= 0x1FD3)
    || (ch >= 0x1FD6 && ch <= 0x1FDB)
    || (ch >= 0x1FE0 && ch <= 0x1FEC)
    || (ch >= 0x1FF2 && ch <= 0x1FF4)
    || (ch >= 0x1FF6 && ch <= 0x1FFC)
    || (ch == 0x207F)
    || (ch == 0x2102)
    || (ch == 0x2107)
    || (ch >= 0x210A && ch <= 0x2113)
    || (ch == 0x2115)
    || (ch >= 0x2119 && ch <= 0x211D)
    || (ch == 0x2124)
    || (ch == 0x2126)
    || (ch == 0x2128)
    || (ch >= 0x212A && ch <= 0x212D)
    || (ch >= 0x212F && ch <= 0x2131)
    || (ch >= 0x2133 && ch <= 0x2139)
    || (ch >= 0x2160 && ch <= 0x2183)
    || (ch >= 0x3005 && ch <= 0x3007)
    || (ch >= 0x3021 && ch <= 0x3029)
    || (ch >= 0x3031 && ch <= 0x3035)
    || (ch >= 0x3038 && ch <= 0x303A)
    || (ch >= 0x3041 && ch <= 0x3094)
    || (ch >= 0x309D && ch <= 0x309E)
    || (ch >= 0x30A1 && ch <= 0x30FA)
    || (ch >= 0x30FC && ch <= 0x30FE)
    || (ch >= 0x3105 && ch <= 0x312C)
    || (ch >= 0x3131 && ch <= 0x318E)
    || (ch >= 0x31A0 && ch <= 0x31B7)
    || (ch == 0x3400)
    || (ch == 0x4DB5)
    || (ch == 0x4E00)
    || (ch == 0x9FA5)
    || (ch >= 0xA000 && ch <= 0xA48C)
    || (ch == 0xAC00)
    || (ch == 0xD7A3)
    || (ch >= 0xF900 && ch <= 0xFA2D)
    || (ch >= 0xFB00 && ch <= 0xFB06)
    || (ch >= 0xFB13 && ch <= 0xFB17)
    || (ch == 0xFB1D)
    || (ch >= 0xFB1F && ch <= 0xFB28)
    || (ch >= 0xFB2A && ch <= 0xFB36)
    || (ch >= 0xFB38 && ch <= 0xFB3C)
    || (ch == 0xFB3E)
    || (ch >= 0xFB40 && ch <= 0xFB41)
    || (ch >= 0xFB43 && ch <= 0xFB44)
    || (ch >= 0xFB46 && ch <= 0xFBB1)
    || (ch >= 0xFBD3 && ch <= 0xFD3D)
    || (ch >= 0xFD50 && ch <= 0xFD8F)
    || (ch >= 0xFD92 && ch <= 0xFDC7)
    || (ch >= 0xFDF0 && ch <= 0xFDFB)
    || (ch >= 0xFE70 && ch <= 0xFE72)
    || (ch == 0xFE74)
    || (ch >= 0xFE76 && ch <= 0xFEFC)
    || (ch >= 0xFF21 && ch <= 0xFF3A)
    || (ch >= 0xFF41 && ch <= 0xFF5A)
    || (ch >= 0xFF66 && ch <= 0xFFBE)
    || (ch >= 0xFFC2 && ch <= 0xFFC7)
    || (ch >= 0xFFCA && ch <= 0xFFCF)
    || (ch >= 0xFFD2 && ch <= 0xFFD7)
    || (ch >= 0xFFDA && ch <= 0xFFDC);
}

static ANTLR3_BOOLEAN isUnicodeDigit(int ch)
{
  // Any character in the Unicode category "Decimal number (Nd)".
  return (ch >= 0x0030 && ch <= 0x0039)
    || (ch >= 0x0660 && ch <= 0x0669)
    || (ch >= 0x06F0 && ch <= 0x06F9)
    || (ch >= 0x0966 && ch <= 0x096F)
    || (ch >= 0x09E6 && ch <= 0x09EF)
    || (ch >= 0x0A66 && ch <= 0x0A6F)
    || (ch >= 0x0AE6 && ch <= 0x0AEF)
    || (ch >= 0x0B66 && ch <= 0x0B6F)
    || (ch >= 0x0BE7 && ch <= 0x0BEF)
    || (ch >= 0x0C66 && ch <= 0x0C6F)
    || (ch >= 0x0CE6 && ch <= 0x0CEF)
    || (ch >= 0x0D66 && ch <= 0x0D6F)
    || (ch >= 0x0E50 && ch <= 0x0E59)
    || (ch >= 0x0ED0 && ch <= 0x0ED9)
    || (ch >= 0x0F20 && ch <= 0x0F29)
    || (ch >= 0x1040 && ch <= 0x1049)
    || (ch >= 0x1369 && ch <= 0x1371)
    || (ch >= 0x17E0 && ch <= 0x17E9)
    || (ch >= 0x1810 && ch <= 0x1819)
    || (ch >= 0xFF10 && ch <= 0xFF19);
}
	
static ANTLR3_BOOLEAN isUnicodeConnectorPunctuation(int ch)
{
  // Any character in the Unicode category "Connector punctuation (Pc)".
  return (ch >= 0x005F)
    || (ch >= 0x203F && ch <= 0x2040)
    || (ch == 0x30FB)
    || (ch >= 0xFE33 && ch <= 0xFE34)
    || (ch >= 0xFE4D && ch <= 0xFE4F)
    || (ch == 0xFF3F)
    || (ch == 0xFF65);
}

static ANTLR3_BOOLEAN isIdentifierStartUnicode(int ch)
{
  return isUnicodeLetter(ch) || (ch == '$') || (ch == '_'); // TODO: add Unicode escape sequence check.
}

static ANTLR3_BOOLEAN isIdentifierPartUnicode(int ch)
{
  return isIdentifierStartUnicode(ch) || isUnicodeDigit(ch) || isUnicodeConnectorPunctuation(ch);
}

static void consumeIdentifierUnicodeStart(pECMALexer ctx)
{
	int ch = LA(1);
	if (isIdentifierStartUnicode(ch))
	{
		MATCHANY();
		do
		{
			ch = LA(1);
			if (ch == '$' || (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') ||
			  ch == '\\' || ch == '_' || (ch >= 'a' && ch <= 'z') || isIdentifierPartUnicode(ch))
			{
				mIdentifierPart(ctx);
			}
			else
			{
				return;
			}
		}
		while (1);
	}
	else
	{
		CONSTRUCTEX();
		EXCEPTION->type      = ANTLR3_NO_VIABLE_ALT_EXCEPTION;
		EXCEPTION->message   = (void *)"isIdentifierStartUnicode";
		EXCEPTION->ruleName  = (void *)"consumeIdentifierUnicodeStart";
	}
}

static pANTLR3_COMMON_TOKEN nextToken(pANTLR3_TOKEN_SOURCE tokenSource)
{
	pECMALexer lexer = (pECMALexer)((pANTLR3_LEXER)tokenSource->super)->ctx;
	pANTLR3_COMMON_TOKEN result = lexer->originalNextToken(tokenSource);
	if (result->channel == ANTLR3_TOKEN_DEFAULT_CHANNEL)
	{
		last = result;
	}
	return result;		
}

}

@lexer::context
{
pANTLR3_COMMON_TOKEN (*originalNextToken)(pANTLR3_TOKEN_SOURCE tokenSource);
}

@lexer::apifuncs
{
 // Install custom error collector for the front end.
 RECOGNIZER->displayRecognitionError = onECMAParseError;

 // Override the nextToken function in the token source.
 pANTLR3_TOKEN_SOURCE tokenSource = TOKENSOURCE(ctx);
 ctx->originalNextToken = tokenSource->nextToken;
 tokenSource->nextToken = nextToken;
}

@parser::members
{

static ANTLR3_BOOLEAN isLeftHandSideExpression(pANTLR3_BASE_TREE lhs)
{
  if (lhs == NULL)
    return ANTLR3_TRUE;

  switch (lhs->getType(lhs))
  {
    case THIS_SYMBOL:
    case Identifier:
    case NULL_SYMBOL:
    case TRUE_SYMBOL:
    case FALSE_SYMBOL:
    case DecimalLiteral:
    case OctalIntegerLiteral:
    case HexIntegerLiteral:
    case StringLiteral:
    case RegularExpressionLiteral:
    case ARRAY_TOKEN:
    case OBJECT_TOKEN:
    case PAREXPR_TOKEN:
    case FUNCTION_SYMBOL:
    case NEW_SYMBOL:
    case CALL_TOKEN:
    case BYFIELD_TOKEN:
    case BYINDEX_TOKEN:
      return ANTLR3_TRUE;

    default:
      return ANTLR3_FALSE;
  }
}
	
ANTLR3_BOOLEAN isLeftHandSideAssign(pECMAParser ctx, pANTLR3_BASE_TREE lhs, int *cached)
{
  if (*cached != -1)
    return *cached;
	
  ANTLR3_BOOLEAN result;
  if (isLeftHandSideExpression(lhs))
  {
    switch (LA(1))
    {
      case ASSIGN_OPERATOR:
      case MULT_ASSIGN_OPERATOR:
      case DIV_ASSIGN_OPERATOR:
      case MOD_ASSIGN_OPERATOR:
      case PLUS_ASSIGN_OPERATOR:
      case MINUS_ASSIGN_OPERATOR:
      case SHIFT_LEFT_ASSIGN_OPERATOR:
      case SHIFT_RIGHT_ASSIGN_OPERATOR:
      case UNSIGNED_SHIFT_RIGHT_ASSIGN_OPERATOR:
      case BITWISE_AND_ASSIGN_OPERATOR:
      case BITWISE_XOR_ASSIGN_OPERATOR:
      case BITWISE_OR_ASSIGN_OPERATOR:
        result = ANTLR3_TRUE;
        break;
      default:
        result = ANTLR3_FALSE;
        break;
      }
    }
    else
      result = ANTLR3_FALSE;

    *cached = result;
    return result;
}

ANTLR3_BOOLEAN isLeftHandSideIn(pECMAParser ctx, pANTLR3_BASE_TREE lhs, int *cached)
{
  if (*cached != -1)
    return *cached;
  
  ANTLR3_BOOLEAN result = isLeftHandSideExpression(lhs) && (LA(1) == IN_SYMBOL);
  *cached = result;
  return result;
}

void promoteEOL(pECMAParser ctx, ECMAParser_semicolon_return *rule)
{
  // Get current token and its type (the possibly offending token).
  pANTLR3_COMMON_TOKEN lt = LT(1);
  int la = lt->getType(lt);
  
  // We only need to promote an EOL when the current token is offending (not a SEMIC, EOF, RBRACE, EOL or MultiLineComment).
  // EOL and MultiLineComment are not offending as they're already promoted in a previous call to this method.
  // Promoting an EOL means switching it from off channel to on channel.
  // A MultiLineComment gets promoted when it contains an EOL.
  if (!(la == SEMICOLON_SYMBOL || la == EOF || la == RIGHT_BRACE_SYMBOL || la == EOL || la == MultiLineComment))
  {
    // Start on the position before the current token and scan backwards off channel tokens until the previous on channel token.
    ANTLR3_INT32 ix;
    for (ix = (ANTLR3_INT32)lt->getTokenIndex(lt) - 1; ix > 0; ix--)
    {
      lt = INPUT->get(INPUT, ix);
      if (lt->getChannel(lt) == 0) // Default channel.
        break;
      else if (lt->getType(lt) == EOL || (lt->getType(lt) == MultiLineComment && matches(lt->getText(lt), "/.*\r\n|\r|\n")))
      {
        // We found our EOL: promote the token to on channel, position the input on it and reset the rule start.
        lt->setChannel(lt, 0);
        SEEK(lt->getTokenIndex(lt));
        if (rule != NULL)
          rule->start = lt;
        break;
      }
    }
  }
}

}

@parser::header {

#define ANTLR3_HUGE
#ifndef _WIN32
  #pragma GCC diagnostic ignored "-Wunused-variable"
  #pragma GCC diagnostic ignored "-Wparentheses"
  #ifdef __APPLE__
    // Comparison of unsigned expression >= 0 is always true.
    #pragma GCC diagnostic ignored "-Wtautological-compare"
  #else
    #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 6 )
      #pragma GCC diagnostic ignored "-Wtype-limits"
    #endif
  #endif
#endif
}

@parser::postinclude {
#ifdef __cplusplus
extern "C" { 
#endif

  // Custom error reporting function.
  void onECMAParseError(struct ANTLR3_BASE_RECOGNIZER_struct *recognizer, pANTLR3_UINT8 *tokenNames); 

  // Regular expression match of text to pattern.
  ANTLR3_BOOLEAN matches(pANTLR3_STRING text, const char *pattern);
  
#ifdef __cplusplus
};
#endif
}

@parser::apifuncs
{
	// Install custom error collector for the front end.
	RECOGNIZER->displayRecognitionError = onECMAParseError;
}

// A.1 Lexical Grammar (7)
// Added for lexing purposes

fragment BACKSLASH_SYMBOL
	: '\\'
	;
	
fragment DOUBLE_QUOTE_SYMBOL
	: '"'
	;
	
fragment SINGLE_QUOTE_SYMBOL
	: '\''
	;

// Whitespace (7.2)

fragment TAB_SYMBOL
	: '\t'
	;

fragment VERTICAL_TAB_SYMBOL // Vertical TAB
	: '\u000b'
	;

fragment FORM_FEED__SYMBOL // Form Feed
	: '\u000c'
	;

fragment SPACE_SYMBOL // Space
	: ' '
	;

fragment NON_BREAKING_SPACE_SYMBOL // Non-Breaking Space
	: '\u00a0'
	;

fragment SPACE_SEPARATORS // Unicode category Zs
	: '\u1680'  // OGHAM SPACE MARK
	| '\u180E'  // MONGOLIAN VOWEL SEPARATOR
	| '\u2000'  // EN QUAD
	| '\u2001'  // EM QUAD
	| '\u2002'  // EN SPACE
	| '\u2003'  // EM SPACE
	| '\u2004'  // THREE-PER-EM SPACE
	| '\u2005'  // FOUR-PER-EM SPACE
	| '\u2006'  // SIX-PER-EM SPACE
	| '\u2007'  // FIGURE SPACE
	| '\u2008'  // PUNCTUATION SPACE
	| '\u2009'  // THIN SPACE
	| '\u200A'  // HAIR SPACE
	| '\u202F'  // NARROW NO-BREAK SPACE
	| '\u205F'  // MEDIUM MATHEMATICAL SPACE
	| '\u3000'  // IDEOGRAPHIC SPACE
;

WhiteSpace
	: ( TAB_SYMBOL | VERTICAL_TAB_SYMBOL | FORM_FEED__SYMBOL | SPACE_SYMBOL | NON_BREAKING_SPACE_SYMBOL | SPACE_SEPARATORS )+ { $channel = HIDDEN; }
	;



// Line terminators (7.3)

fragment LINE_FEED_SYMBOL // Line Feed
	: '\n'
	;

fragment CARRIAGE_RETURN_SYMBOL // Carriage Return
	: '\r'
	;

fragment LINE_SEPARATOR_SYMBOL // Line Separator
	: '\u2028'
	;

fragment PARAGRAPH_SEPARATOR_SYMBOL // Paragraph Separator
	: '\u2029'
	;

fragment LINE_TERMINATOR
	: CARRIAGE_RETURN_SYMBOL | LINE_FEED_SYMBOL | LINE_SEPARATOR_SYMBOL | PARAGRAPH_SEPARATOR_SYMBOL
	;
		
EOL
	: ( ( CARRIAGE_RETURN_SYMBOL LINE_FEED_SYMBOL? ) | LINE_FEED_SYMBOL | LINE_SEPARATOR_SYMBOL | PARAGRAPH_SEPARATOR_SYMBOL ) { $channel = HIDDEN; }
	;

// Comments (7.4)

MultiLineComment
	: '/*' ( options { greedy = false; } : . )* '*/' { $channel = HIDDEN; }
	;

SingleLineComment
	: '//' ( ~( LINE_TERMINATOR ) )* { $channel = HIDDEN; }
	;



// Tokens (7.5)

token
	: reservedWord
	| Identifier
	| punctuator
	| numericLiteral
	| StringLiteral
	;

// Reserved words (7.5.1)

reservedWord
	: keyword
	| futureReservedWord
	| NULL_SYMBOL
	| booleanLiteral
	;

// Keywords (7.5.2)

keyword
	: BREAK_SYMBOL
	| CASE_SYMBOL
	| CATCH_SYMBOL
	| CONTINUE_SYMBOL
	| DEFAULT_SYMBOL
	| DELETE_SYMBOL
	| DO_SYMBOL
	| ELSE_SYMBOL
	| FINALLY_SYMBOL
	| FOR_SYMBOL
	| FUNCTION_SYMBOL
	| IF_SYMBOL
	| IN_SYMBOL
	| INSTANCEOF_SYMBOL
	| NEW_SYMBOL
	| RETURN_SYMBOL
	| SWITCH_SYMBOL
	| THIS_SYMBOL
	| THROW_SYMBOL
	| TRY_SYMBOL
	| TYPEOF_SYMBOL
	| VAR_SYMBOL
	| VOID_SYMBOL
	| WHILE_SYMBOL
	| WITH_SYMBOL
	;



// Future reserved words (7.5.3)

futureReservedWord
	: ABSTRACT_SYMBOL
	| BOOLEAN_SYMBOL
	| BYTE_SYMBOL
	| CHAR_SYMBOL
	| CLASS_SYMBOL
	| CONST_SYMBOL
	| DEBUGGER_SYMBOL
	| DOUBLE_SYMBOL
	| ENUM_SYMBOL
	| EXPORT_SYMBOL
	| EXTENDS_SYMBOL
	| FINAL_SYMBOL
	| FLOAT_SYMBOL
	| GOTO_SYMBOL
	| IMPLEMENTS_SYMBOL
	| IMPORT_SYMBOL
	| INT_SYMBOL
	| INTERFACE_SYMBOL
	| LONG_SYMBOL
	| NATIVE_SYMBOL
	| PACKAGE_SYMBOL
	| PRIVATE_SYMBOL
	| PROTECTED_SYMBOL
	| PUBLIC_SYMBOL
	| SHORT_SYMBOL
	| STATIC_SYMBOL
	| SUPER_SYMBOL
	| SYNCHRONIZED_SYMBOL
	| THROWS_SYMBOL
	| TRANSIENT_SYMBOL
	| VOLATILE_SYMBOL
	;

// Identifiers (7.6)

fragment IdentifierStartASCII
	: 'a'..'z' | 'A'..'Z'
	| '$'
	| '_'
	| BACKSLASH_SYMBOL 'u' HexDigit HexDigit HexDigit HexDigit // Unicode escape sequence
	;

/*
 The first two alternatives define how ANTLR can match ASCII characters which can be considered as part of an identifier.
 The last alternative matches other characters in the unicode range that can be sonsidered as part of an identifier.
*/
fragment IdentifierPart
	: DECIMAL_DIGIT
	| IdentifierStartASCII
	| { isIdentifierPartUnicode(LA(1)) }? { MATCHANY(); }
	;

fragment IdentifierNameASCIIStart
	: IdentifierStartASCII IdentifierPart*
	;

/*
 The second alternative acts as an action driven fallback to evaluate other characters in the unicode range than the ones in the ASCII subset.
 Due to the first alternative this grammar defines enough so that ANTLR can generate a lexer that correctly predicts identifiers with characters in the ASCII range.
 In that way keywords, other reserved words and ASCII identifiers are recognized with standard ANTLR driven logic. When the first character for an identifier fails to 
 match this ASCII definition, the lexer calls consumeIdentifierUnicodeStart because of the action in the alternative. This method checks whether the character matches 
 as first character in ranges other than ASCII and consumes further characters belonging to the identifier with help of mIdentifierPart generated out of the
 IdentifierPart rule above.
*/
Identifier
	: IdentifierNameASCIIStart
	| { consumeIdentifierUnicodeStart(ctx); }
	;


// Punctuators (7.7)

punctuator
	: LEFT_BRACE_SYMBOL
	| RIGHT_BRACE_SYMBOL
	| LEFT_PAR_SYMBOL
	| RIGHT_PAR_SYMBOL
	| LEFT_BRACKET_SYMBOL
	| RIGHT_BRACKET_SYMBOL
	| DOT_SYMBOL
	| SEMICOLON_SYMBOL
	| COMMA_SYMBOL
	| LESS_THAN_OPERATOR
	| GREATER_THAN_OPERATOR
	| LESS_OR_EQUAL_OPERATOR
	| GREATER_OR_EQUAL_OPERATOR
	| EQUAL_OPERATOR
	| NOT_EQUAL_OPERATOR
	| SAME_OPERATOR
	| NOT_SAME_OPERATOR
	| PLUS_OPERATOR
	| MINUS_OPERATOR
	| MULT_OPERATOR
	| MODULO_OPERATOR
	| INCREMENT_OPERATOR
	| DECREMENT_OPERATOR
	| SHIFT_LEFT_OPERATOR
	| SHIFT_RIGHT_OPERATOR
	| UNSIGNED_SHIFT_RIGHT_OPERATOR
	| BITWISE_AND_OPERATOR
	| BITWISE_OR_OPERATOR
	| BITWISE_XOR_OPERATOR
	| LOGICAL_NOT_OPERATOR
	| BITWISE_NOT_OPERATOR
	| LOGICAL_AND_OPERATOR
	| LOGICAL_OR_OPERATOR
	| QUESTION_MARK_SYMBOL
	| COLON_SYMBOL
	| ASSIGN_OPERATOR
	| PLUS_ASSIGN_OPERATOR
	| MINUS_ASSIGN_OPERATOR
	| MULT_ASSIGN_OPERATOR
	| MOD_ASSIGN_OPERATOR
	| SHIFT_LEFT_ASSIGN_OPERATOR
	| SHIFT_RIGHT_ASSIGN_OPERATOR
	| UNSIGNED_SHIFT_RIGHT_ASSIGN_OPERATOR
	| BITWISE_AND_ASSIGN_OPERATOR
	| BITWISE_OR_ASSIGN_OPERATOR
	| BITWISE_XOR_ASSIGN_OPERATOR
	| DIV_OPERATOR
	| DIV_ASSIGN_OPERATOR
	;

// Literals (7.8)

literal
	: NULL_SYMBOL
	| booleanLiteral
	| numericLiteral
	| StringLiteral
	| RegularExpressionLiteral
	;

booleanLiteral
	: TRUE_SYMBOL
	| FALSE_SYMBOL
	;

// Numeric literals (7.8.3)

/*
Note: octal literals are described in the B Compatibility section.
These are removed from the standards but are here for backwards compatibility with earlier ECMAScript definitions.
*/

fragment DECIMAL_DIGIT
	: '0'..'9'
	;

fragment HexDigit
	: DECIMAL_DIGIT | 'a'..'f' | 'A'..'F'
	;

fragment OctalDigit
	: '0'..'7'
	;

fragment ExponentPart
	: ( 'e' | 'E' ) ( '+' | '-' )? DECIMAL_DIGIT+
	;

fragment DecimalIntegerLiteral
	: '0'
	| '1'..'9' DECIMAL_DIGIT*
	;

DecimalLiteral
	: DecimalIntegerLiteral '.' DECIMAL_DIGIT* ExponentPart?
	| '.' DECIMAL_DIGIT+ ExponentPart?
	| DecimalIntegerLiteral ExponentPart?
	;

OctalIntegerLiteral
	: '0' OctalDigit+
	;

HexIntegerLiteral
	: ( '0x' | '0X' ) HexDigit+
	;

numericLiteral
	: DecimalLiteral
	| OctalIntegerLiteral
	| HexIntegerLiteral
	;


// String literals (7.8.4)

fragment CHAR_ESCAPE_SEQUENCE
	: ~( DECIMAL_DIGIT | 'x' | 'u' | LINE_TERMINATOR ) // Concatenation of SingleEscapeCharacter and NonEscapeCharacter
	;

fragment ZeroToThree
	: '0'..'3'
	;
	
fragment HexEscapeSequence
	: 'x' HexDigit HexDigit
	;
	
fragment UnicodeEscapeSequence
	: 'u' HexDigit HexDigit HexDigit HexDigit
	;

fragment ESCAPE_SEQUENCE
	:
	BACKSLASH_SYMBOL 
	(
		CHAR_ESCAPE_SEQUENCE 
		| HexEscapeSequence
		| UnicodeEscapeSequence
	)
	;

StringLiteral
	: SINGLE_QUOTE_SYMBOL ( ~( SINGLE_QUOTE_SYMBOL | BACKSLASH_SYMBOL | LINE_TERMINATOR ) | ESCAPE_SEQUENCE )* SINGLE_QUOTE_SYMBOL
	| DOUBLE_QUOTE_SYMBOL ( ~( DOUBLE_QUOTE_SYMBOL | BACKSLASH_SYMBOL | LINE_TERMINATOR ) | ESCAPE_SEQUENCE )* DOUBLE_QUOTE_SYMBOL
	;


// Regular expression literals (7.8.5)

fragment BackslashSequence
	: BACKSLASH_SYMBOL ~( LINE_TERMINATOR )
	;

fragment RegularExpressionFirstChar
	: ~ ( LINE_TERMINATOR | MULT_OPERATOR | BACKSLASH_SYMBOL | DIV_OPERATOR )
	| BackslashSequence
	;

fragment RegularExpressionChar
	: ~ ( LINE_TERMINATOR | BACKSLASH_SYMBOL | DIV_OPERATOR )
	| BackslashSequence
	;

RegularExpressionLiteral
	: { regex_enabled(ctx) }? => DIV_OPERATOR RegularExpressionFirstChar RegularExpressionChar* DIV_OPERATOR IdentifierPart*
	;

// A.3 Expressions (11)

// Primary expressions (11.1)

primaryExpression
	: THIS_SYMBOL
	| Identifier
	| literal
	| arrayLiteral
	| objectLiteral
	| lpar = LEFT_PAR_SYMBOL expression RIGHT_PAR_SYMBOL -> ^( PAREXPR_TOKEN[$lpar, "PAREXPR"] expression )
	;

arrayLiteral
	: lb = LEFT_BRACKET_SYMBOL (arrayItem (COMMA_SYMBOL arrayItem)*)? RIGHT_BRACKET_SYMBOL
		-> ^( ARRAY_TOKEN[$lb, "ARRAY"] arrayItem* )
	;

arrayItem
	: ( expr = assignmentExpression | { LA(1) == COMMA_SYMBOL }? ) -> ^( ITEM_TOKEN $expr? )
;

objectLiteral
	: lb=LEFT_BRACE_SYMBOL ( nameValuePair ( COMMA_SYMBOL nameValuePair )* )? RIGHT_BRACE_SYMBOL
	-> ^( OBJECT_TOKEN[$lb, "OBJECT"] nameValuePair* )
	;
	
nameValuePair
	: propertyName COLON_SYMBOL assignmentExpression
	-> ^( NAMED_VALUE_TOKEN propertyName assignmentExpression )
	;

propertyName
	: memberName
	| StringLiteral
	| numericLiteral
	;

// Left-hand-side expressions (11.2)

/*
  Refactored some rules to make them LL(*) compliant:
    All the expressions surrounding member selection and calls have been moved to leftHandSideExpression
    to make them right recursive.
*/

memberExpression
	: primaryExpression
	| functionExpression
	| newExpression
	;

newExpression
	: NEW_SYMBOL^ memberExpression
	;

	
arguments
	: LEFT_PAR_SYMBOL ( assignmentExpression ( COMMA_SYMBOL assignmentExpression )* )? RIGHT_PAR_SYMBOL
	-> ^( ARGS_TOKEN assignmentExpression* )
	;
	
leftHandSideExpression
	:
	(
		memberExpression -> memberExpression
	)
	(
		arguments -> ^( CALL_TOKEN $leftHandSideExpression arguments )
		| LEFT_BRACKET_SYMBOL expression RIGHT_BRACKET_SYMBOL -> ^( BYINDEX_TOKEN $leftHandSideExpression expression )
		| DOT_SYMBOL memberName -> ^( BYFIELD_TOKEN $leftHandSideExpression memberName )
	)*
	;

memberName:
	Identifier
	| keyword
;

// Postfix expressions (11.3)

/*
  The specification states that there are no line terminators allowed before the postfix operators.
  This is enforced by the call to promoteEOL in the action before ( INC | DEC ).
  We only must promote EOLs when the la is INC or DEC because this production is chained as all expression rules.
  In other words: only promote EOL when we are really in a postfix expression. A check on the la will ensure this.
*/
postfixExpression
	: leftHandSideExpression { if (LA(1) == INCREMENT_OPERATOR || LA(1) == DECREMENT_OPERATOR) promoteEOL(ctx, NULL); } ( postfixOperator^ )?
	;
	
postfixOperator
	: op = INCREMENT_OPERATOR { $op->setType($op, PINC_TOKEN); }
	| op = DECREMENT_OPERATOR { $op->setType($op, PDEC_TOKEN); }
	;

// Unary operators (11.4)

unaryExpression
	: postfixExpression
	| unaryOperator^ unaryExpression
	;
	
unaryOperator
	: DELETE_SYMBOL
	| VOID_SYMBOL
	| TYPEOF_SYMBOL
	| INCREMENT_OPERATOR
	| DECREMENT_OPERATOR
	| op = PLUS_OPERATOR { $op->setType($op, POS_TOKEN); }
	| op = MINUS_OPERATOR { $op->setType($op, NEG_TOKEN); }
	| BITWISE_NOT_OPERATOR
	| LOGICAL_NOT_OPERATOR
	;

// Multiplicative operators (11.5)

multiplicativeExpression
	: unaryExpression ( ( MULT_OPERATOR | DIV_OPERATOR | MODULO_OPERATOR )^ unaryExpression )*
	;

// Additive operators (11.6)

additiveExpression
	: multiplicativeExpression ( ( PLUS_OPERATOR | MINUS_OPERATOR )^ multiplicativeExpression )*
	;


// Bitwise shift operators (11.7)

shiftExpression
	: additiveExpression ( ( SHIFT_LEFT_OPERATOR | SHIFT_RIGHT_OPERATOR | UNSIGNED_SHIFT_RIGHT_OPERATOR )^ additiveExpression )*
	;

// Relational operators (11.8)

relationalExpression
	: shiftExpression ( ( LESS_THAN_OPERATOR | GREATER_THAN_OPERATOR | LESS_OR_EQUAL_OPERATOR | GREATER_OR_EQUAL_OPERATOR | INSTANCEOF_SYMBOL | IN_SYMBOL )^ shiftExpression )*
	;

relationalExpressionNoIn
	: shiftExpression ( ( LESS_THAN_OPERATOR | GREATER_THAN_OPERATOR | LESS_OR_EQUAL_OPERATOR | GREATER_OR_EQUAL_OPERATOR | INSTANCEOF_SYMBOL )^ shiftExpression )*
	;

// Equality operators (11.9)

equalityExpression
	: relationalExpression ( ( EQUAL_OPERATOR | NOT_EQUAL_OPERATOR | SAME_OPERATOR | NOT_SAME_OPERATOR )^ relationalExpression )*
	;

equalityExpressionNoIn
	: relationalExpressionNoIn ( ( EQUAL_OPERATOR | NOT_EQUAL_OPERATOR | SAME_OPERATOR | NOT_SAME_OPERATOR )^ relationalExpressionNoIn )*
	;
	
// Binary bitwise operators (11.10)

bitwiseANDExpression
	: equalityExpression ( BITWISE_AND_OPERATOR^ equalityExpression )*
	;

bitwiseANDExpressionNoIn
	: equalityExpressionNoIn ( BITWISE_AND_OPERATOR^ equalityExpressionNoIn )*
	;
		
bitwiseXORExpression
	: bitwiseANDExpression ( BITWISE_XOR_OPERATOR^ bitwiseANDExpression )*
	;
		
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn ( BITWISE_XOR_OPERATOR^ bitwiseANDExpressionNoIn )*
	;
	
bitwiseORExpression
	: bitwiseXORExpression ( BITWISE_OR_OPERATOR^ bitwiseXORExpression )*
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn ( BITWISE_OR_OPERATOR^ bitwiseXORExpressionNoIn )*
	;

// Binary logical operators (11.11)

logicalANDExpression
	: bitwiseORExpression ( LOGICAL_AND_OPERATOR^ bitwiseORExpression )*
	;

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn ( LOGICAL_AND_OPERATOR^ bitwiseORExpressionNoIn )*
	;
	
logicalORExpression
	: logicalANDExpression ( LOGICAL_OR_OPERATOR^ logicalANDExpression )*
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn ( LOGICAL_OR_OPERATOR^ logicalANDExpressionNoIn )*
	;

// Conditional operator (11.12)

conditionalExpression
	: logicalORExpression ( QUESTION_MARK_SYMBOL^ assignmentExpression COLON_SYMBOL! assignmentExpression )?
	;

conditionalExpressionNoIn
	: logicalORExpressionNoIn ( QUESTION_MARK_SYMBOL^ assignmentExpressionNoIn COLON_SYMBOL! assignmentExpressionNoIn )?
	;

// Assignment operators (11.13)

/*
The specification defines the AssignmentExpression rule as follows:
AssignmentExpression :
	ConditionalExpression 
	LeftHandSideExpression AssignmentOperator AssignmentExpression
This rule has a LL(*) conflict. Resolving this with a syntactical predicate will yield something like this:

assignmentExpression
	: ( leftHandSideExpression assignmentOperator )=> leftHandSideExpression assignmentOperator^ assignmentExpression
	| conditionalExpression
	;
assignmentOperator
	: ASSIGN | MULASS | DIVASS | MODASS | ADDASS | SUBASS | SHLASS | SHRASS | SHUASS | ANDASS | XORASS | ORASS
	;
	
But that didn't seem to work. Terence Par writes in his book that LL(*) conflicts in general can best be solved with auto backtracking. But that would be 
a performance killer for such a heavy used rule.
The solution I came up with is to always invoke the conditionalExpression first and than decide what to do based on the result of that rule.
When the rule results in a Tree that can't be coming from a left hand side expression, then we're done.
When it results in a Tree that is coming from a left hand side expression and the LA(1) is an assignment operator then parse the assignment operator
followed by the right recursive call.
*/
assignmentExpression
@init
{
	int isLhs = -1; // TODO: this doesn't do anything, except taking the result of isLeftHandSideAssign (which is then never used again).
}
	: lhs = conditionalExpression
	( { isLeftHandSideAssign(ctx, lhs.tree, &isLhs) }? assignmentOperator^ assignmentExpression )?	
	;

assignmentOperator:
	ASSIGN_OPERATOR
	| MULT_ASSIGN_OPERATOR
	| DIV_ASSIGN_OPERATOR
	| MOD_ASSIGN_OPERATOR
	| PLUS_ASSIGN_OPERATOR
	| MINUS_ASSIGN_OPERATOR
	| SHIFT_LEFT_ASSIGN_OPERATOR
	| SHIFT_RIGHT_ASSIGN_OPERATOR
	| UNSIGNED_SHIFT_RIGHT_ASSIGN_OPERATOR
	| BITWISE_AND_ASSIGN_OPERATOR
	| BITWISE_XOR_ASSIGN_OPERATOR
	| BITWISE_OR_ASSIGN_OPERATOR
;

assignmentExpressionNoIn
@init
{
	int isLhs = -1;
}
	: lhs = conditionalExpressionNoIn
	( { isLeftHandSideAssign(ctx, lhs.tree, &isLhs) }? assignmentOperator^ assignmentExpressionNoIn )?
	;

// Comma operator (11.14)

expression
	: exprs += assignmentExpression ( COMMA_SYMBOL exprs+=assignmentExpression )*
	-> { $exprs->size($exprs) > 1 }? ^( CEXPR_TOKEN $exprs+ )
	-> $exprs
	;

expressionNoIn
	: exprs += assignmentExpressionNoIn ( COMMA_SYMBOL exprs += assignmentExpressionNoIn )*
	-> { $exprs->size($exprs) > 1 }? ^( CEXPR_TOKEN $exprs+ )
	-> $exprs
	;

// A.4 Statements (12)
//

/*
  This rule handles semicolons reported by the lexer and situations where the ECMA 3 specification states there should be semicolons automaticly inserted.
  The auto semicolons are not actually inserted but this rule behaves as if they were.

  In the following situations an ECMA 3 parser should auto insert absent but grammatically required semicolons:
    - the current token is a right brace
    - the current token is the end of file (EOF) token
    - there is at least one end of line (EOL) token between the current token and the previous token.

  The RBRACE is handled by matching it but not consuming it.
  The EOF needs no further handling because it is not consumed by default.
  The EOL situation is handled by promoting the EOL or MultiLineComment with an EOL present from off channel to on channel
  and thus making it parseable instead of handling it as white space. This promoting is done in the action promoteEOL.
*/
semicolon
@init
{
	// Mark current position so we can unconsume an RBRACE.
	ANTLR3_MARKER marker = MARK();
	// Promote EOL if appropriate	
	promoteEOL(ctx, &retval);
}
	: SEMICOLON_SYMBOL
	| EOF
	| RIGHT_BRACE_SYMBOL { REWIND(marker); }
	| EOL
	| MultiLineComment // (with EOL in it)
	;

/*
  To solve the ambiguity between block and objectLiteral via expressionStatement all but the block alternatives have been moved to statementTail.
  Now when k = 1 and a semantical predicate is defined ANTLR generates code that always will prefer block when the LA(1) is a LBRACE.
  This will result in the same behaviour that is described in the specification under 12.4 on the expressionStatement rule.
*/
statement
options
{
	k = 1;
}
	: { LA(1) == LEFT_BRACE_SYMBOL }? block
	| statementTail
	;
	
statementTail
	: variableStatement
	| emptyStatement
	| expressionStatement
	| ifStatement
	| iterationStatement
	| continueStatement
	| breakStatement
	| returnStatement
	| withStatement
	| labelledStatement
	| switchStatement
	| throwStatement
	| tryStatement
	;

// Block (12.1)

block
	: lb = LEFT_BRACE_SYMBOL statement* RIGHT_BRACE_SYMBOL
	//-> ^( BLOCK_TOKEN[$lb, "BLOCK"] statement* )
	-> ^(BLOCK_TOKEN LEFT_BRACE_SYMBOL statement* RIGHT_BRACE_SYMBOL)
;

// Variable statement 12.2)

variableStatement
	: VAR_SYMBOL^ variableDeclaration ( COMMA_SYMBOL variableDeclaration )* semicolon
	//-> ^( VAR_SYMBOL variableDeclaration+ )
	;

variableDeclaration
	: Identifier ( ASSIGN_OPERATOR^ assignmentExpression )?
	;
	
variableDeclarationNoIn
	: Identifier ( ASSIGN_OPERATOR^ assignmentExpressionNoIn )?
	;

// Empty statement (12.3)

emptyStatement
	: SEMICOLON_SYMBOL
	;

// Expression statement (12.4)

/*
  The look ahead check on LBRACE and FUNCTION the specification mentions has been left out and its function, resolving the ambiguity between:
    - functionExpression and functionDeclaration
    - block and objectLiteral
  are moved to the statement and sourceElement rules.
*/
expressionStatement
	: expression semicolon
	-> ^(expression semicolon)
;

// The if statement (12.5)

ifStatement
// The predicate is there just to get rid of the warning. ANTLR will handle the dangling else just fine.
	: IF_SYMBOL^ LEFT_PAR_SYMBOL expression RIGHT_PAR_SYMBOL statement ( { LA(1) == ELSE_SYMBOL }? ELSE_SYMBOL statement )?
	//-> ^( IF_SYMBOL expression statement+ )
	;

// Iteration statements (12.6)

iterationStatement
	: doStatement
	| whileStatement
	| forStatement
	;
	
doStatement
	: DO_SYMBOL^ statement WHILE_SYMBOL LEFT_PAR_SYMBOL expression RIGHT_PAR_SYMBOL semicolon
	//-> ^( DO_SYMBOL statement expression )
	;
	
whileStatement
	: WHILE_SYMBOL^ LEFT_PAR_SYMBOL expression RIGHT_PAR_SYMBOL statement
	;


forStatement
	: FOR_SYMBOL^ LEFT_PAR_SYMBOL forControl RIGHT_PAR_SYMBOL statement
	;

forControl
	: forControlVar
	| forControlExpression
	| forControlSemic
	;

forControlVar
	: VAR_SYMBOL variableDeclarationNoIn
	(
		(
			IN_SYMBOL expression
			-> ^( FORITER_TOKEN ^( VAR_SYMBOL variableDeclarationNoIn ) ^( EXPR_TOKEN expression ) )
		)
		|
		(
			( COMMA_SYMBOL variableDeclarationNoIn )* SEMICOLON_SYMBOL ex1=expression? SEMICOLON_SYMBOL ex2=expression?
			-> ^( FORSTEP_TOKEN ^( VAR_SYMBOL variableDeclarationNoIn+ ) ^( EXPR_TOKEN $ex1? ) ^( EXPR_TOKEN $ex2? ) )
		)
	)
	;

forControlExpression
@init
{
	int isLhs = -1;
}
	: ex1=expressionNoIn
	( 
		{ isLeftHandSideIn(ctx, ex1.tree, &isLhs) }? (
			IN_SYMBOL ex2 = expression
			-> ^( FORITER_TOKEN ^( EXPR_TOKEN $ex1 ) ^( EXPR_TOKEN $ex2 ) )
		)
		|
		(
			SEMICOLON_SYMBOL ex2 = expression? SEMICOLON_SYMBOL ex3 = expression?
			-> ^( FORSTEP_TOKEN ^( EXPR_TOKEN $ex1 ) ^( EXPR_TOKEN $ex2? ) ^( EXPR_TOKEN $ex3? ) )
		)
	)
	;

forControlSemic
	: SEMICOLON_SYMBOL ex1 = expression? SEMICOLON_SYMBOL ex2=expression?
	-> ^( FORSTEP_TOKEN ^( EXPR_TOKEN ) ^( EXPR_TOKEN $ex1? ) ^( EXPR_TOKEN $ex2? ) )
	;

// The continue statement (12.7)

/*
The action with the call to promoteEOL after CONTINUE is to enforce the semicolon insertion rule of the specification that there are
no line terminators allowed beween CONTINUE and the optional identifier.
As an optimization we check the la first to decide whether there is an identier following.
*/
continueStatement
	: CONTINUE_SYMBOL^ { if (LA(1) == Identifier) promoteEOL(ctx, NULL); } Identifier? semicolon
	;

//The break statement (12.8)

/*
The action with the call to promoteEOL after BREAK is to enforce the semicolon insertion rule of the specification that there are
no line terminators allowed beween BREAK and the optional identifier.
As an optimization we check the la first to decide whether there is an identier following.
*/
breakStatement
	: BREAK_SYMBOL^ { if (LA(1) == Identifier) promoteEOL(ctx, NULL); } Identifier? semicolon
	;

// The return statement (12.9)

/*
The action calling promoteEOL after RETURN ensures that there are no line terminators between RETURN and the optional expression as the specification states.
When there are these get promoted to on channel and thus virtual semicolon wannabees.
So the folowing code:

return
1

will be parsed as:

return;
1;
*/
returnStatement
	: RETURN_SYMBOL^ { promoteEOL(ctx, NULL); } expression? semicolon
	;

// The with statement (12.10)

withStatement
	: WITH_SYMBOL^ LEFT_PAR_SYMBOL! expression RIGHT_PAR_SYMBOL! statement
	;

// The switch statement (12.11)

switchStatement
@init
{
	int defaultClauseCount = 0;
}
	: SWITCH_SYMBOL^ LEFT_PAR_SYMBOL expression RIGHT_PAR_SYMBOL LEFT_BRACE_SYMBOL ( { defaultClauseCount == 0 }? => defaultClause { defaultClauseCount++; } | caseClause )* RIGHT_BRACE_SYMBOL
	//-> ^( SWITCH_SYMBOL expression defaultClause? caseClause* )
	;

caseClause
	: CASE_SYMBOL^ expression COLON_SYMBOL! statement*
	;
	
defaultClause
	: DEFAULT_SYMBOL^ COLON_SYMBOL! statement*
	;

// Labelled statements (12.12)

labelledStatement
	: Identifier COLON_SYMBOL statement
	-> ^( LABELLED_TOKEN Identifier statement )
	;

// The throw statement (12.13)

/*
The action calling promoteEOL after THROW ensures that there are no line terminators between THROW and the expression as the specification states.
When there are line terminators these get promoted to on channel and thus to virtual semicolon wannabees.
So the folowing code:

throw
new Error()

will be parsed as:

throw;
new Error();

which will yield a recognition exception!
*/
throwStatement
	: THROW_SYMBOL^ { promoteEOL(ctx, NULL); } expression semicolon
	;

// The try statement (12.14)

tryStatement
	: TRY_SYMBOL^ block ( catchClause finallyClause? | finallyClause )
	;
	
catchClause
	: CATCH_SYMBOL^ LEFT_PAR_SYMBOL! Identifier RIGHT_PAR_SYMBOL! block
	;
	
finallyClause
	: FINALLY_SYMBOL^ block
	;

// A.5 Functions and Programs (13, 14)

// Function Definition (13)

functionDeclaration
	: FUNCTION_SYMBOL name = Identifier formalParameterList functionBody
		-> ^( FUNCTION_SYMBOL $name formalParameterList functionBody )
	;

functionExpression
	: FUNCTION_SYMBOL name = Identifier? formalParameterList functionBody
		-> ^( FUNCTION_SYMBOL $name? formalParameterList functionBody )
	;

formalParameterList
	: LEFT_PAR_SYMBOL ( Identifier ( COMMA_SYMBOL Identifier )* )? RIGHT_PAR_SYMBOL
	//-> ^( ARGS_TOKEN Identifier* )
	;

functionBody
	: lb = LEFT_BRACE_SYMBOL sourceElement* RIGHT_BRACE_SYMBOL
		//-> ^( BLOCK_TOKEN[$lb, "BLOCK"] sourceElement* )
		-> ^( BLOCK_TOKEN LEFT_BRACE_SYMBOL sourceElement* RIGHT_BRACE_SYMBOL )
	;

// Program (14)

program:
	sourceElement* EOF
;

/*
  By setting k  to 1 for this rule and adding the semantical predicate ANTRL will generate code that will always prefer functionDeclararion over functionExpression
  here and therefor remove the ambiguity between these to production.
  This will result in the same behaviour that is described in the specification under 12.4 on the expressionStatement rule.
*/
sourceElement
options
{
	k = 1;
}
	: { LA(1) == FUNCTION_SYMBOL }? functionDeclaration
	| statement
	;
