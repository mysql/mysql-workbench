# Test file for syntax errors.

# ANTLR3_RECOGNITION_EXCEPTION (parser)
`test` select 1; # Syntax error: `test` (back thick quoted id) is not valid input at this position
'test' select 1; # Syntax error: 'test' (single quoted text) is not valid input at this position
"test" select 1; # Syntax error: "test" (double quoted text) is not valid input at this position
\n select 1; # Syntax error: '\n' (<invalid>) is not valid input at this position
\N select 1; # Syntax error: '\N' (null escape sequence) is not valid input at this position
|| select 1; # Syntax error: '||' (logical or operator) is not valid input at this position

# ANTLR3_NO_VIABLE_ALT_EXCEPTION
select 1 from ; # Syntax error: unexpected end of statement
select 1 from ''; # Syntax error: unexpected '' (single quoted text)

# ANTLR3_MISMATCHED_SET_EXCEPTION
# no test cases yet

# ANTLR3_EARLY_EXIT_EXCEPTION
select a, b,
    (case y
        -- missing when clause here
        then c # Syntax error: missing subclause or elements before 'then' (then)
        else 0 # Syntax error: missing closing parenthesis
    end)
;
 
# ANTLR3_FAILED_PREDICATE_EXCEPTION
# no test cases yet

# ANTLR3_MISMATCHED_TREE_NODE_EXCEPTION
# no need for test cases, just here for completeness

# ANTLR3_REWRITE_EARLY_EXCEPTION
# no need for test cases, just here for completeness

# ANTLR3_UNWANTED_TOKEN_EXCEPTION
select 1 << 2 f a; # Syntax error: extraneous input found - expected end of statement

# ANTLR3_MISSING_TOKEN_EXCEPTION
select (1, 10 "v c" from actor; # Syntax error:  missing closing parenthesis

# ANTLR3_RECOGNITION_EXCEPTION (lexer)
# no need for test cases, all input is currently accepted by the lexer

# Other lexer errors -> must be at the end of the file to not hide other queries.
select 1 'abc; # Syntax error:  unexpected end of input (unfinished string or quoted identifier)

