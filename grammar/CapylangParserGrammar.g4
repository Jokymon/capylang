parser grammar CapylangParserGrammar;
options { tokenVocab=CapylangLexerGrammar; }

module
    : (
        function_import
        | global_definition
        | record_definition
        | function_definition
      )*
      EOF
    ;

global_definition
    : KW_GLOBAL KW_MUT?
        IDENTIFIER COLON type_reference 
        EQ expression SEMI
    ;

record_definition
    : KW_RECORD IDENTIFIER
        LCURLY
            ( IDENTIFIER COLON type_reference COMMA )*
        RCURLY SEMI
    ;

function_import
    : KW_IMPORT IDENTIFIER DCOLON IDENTIFIER function_signature ( OP_CONV IDENTIFIER )? SEMI
    ;

function_definition
    : attribute_definition*
      KW_FN IDENTIFIER function_signature ( ARROW IDENTIFIER )?
      LCURLY
        body
      RCURLY
    ;

function_signature
    : LPAREN
        ( function_argument_decl ( COMMA function_argument_decl )* )?
      RPAREN ( ARROW type_reference )?
    ;

function_argument_decl
    : IDENTIFIER COLON type_reference
    ;

attribute_definition
    : AT IDENTIFIER
      ( LPAREN
        ( IDENTIFIER EQ expression ( COMMA IDENTIFIER EQ expression )* )?
      RPAREN )?
    ;

expression
    : assignment_expression
    ;

assignment_expression
    : comparison_expression ( EQ assignment_expression )?
    ;

comparison_expression
    : bitwise_expression ( ( NEQ | EQEQ | LTE | LT | GTE | GT ) bitwise_expression )*
    ;

bitwise_expression
    : shift_expression ( ( BARBAR | AMPAMP ) shift_expression )*
    ;

shift_expression
    : additive_expression ( ( SHL | SHR ) additive_expression )*
    ;

additive_expression
    : multiplicate_expression ( ( OP_PLUS | OP_MINUS ) multiplicate_expression )*
    ;

multiplicate_expression
    : cast_expression ( ( STAR | OP_DIV | OP_MOD ) cast_expression )*
    ;

cast_expression
    : unary_expression ( OP_CONV type_reference )?
    ;

unary_expression
    : (OP_MINUS | STAR) unary_expression
    | atom
    ;

atom
    : IDENTIFIER LPAREN ( expression ( COMMA expression )* )? RPAREN
    | LPAREN expression RPAREN
    | block_expression
    | record_initialisation
    | IDENTIFIER ( DOT IDENTIFIER )*
    | STRING_LITERAL
    | CHAR_LITERAL
    | number
    ;

block_expression
    : while_expression
    | if_expression
    ;

non_block_expression
    : let_expression
    | assignment_expression
    | KW_BREAK
    | KW_RETURN expression?
    ;

let_expression
    : KW_LET KW_MUT? IDENTIFIER ( COLON type_reference )? ( EQ expression )?
    ;

while_expression
    : KW_WHILE expression LCURLY body RCURLY
    ;

if_expression
    : KW_IF expression LCURLY body RCURLY
      ( KW_ELSE LCURLY body RCURLY )?
    ;

body
    : body_item*
      final_expression?
      SEMI?
    ;

body_item
    : block_expression
    | non_block_expression SEMI
    ;

final_expression
    : expression
    ;

record_initialisation
    : KW_ALLOCATE?
      IDENTIFIER LCURLY
        ( IDENTIFIER EQ expression COMMA )*
      RCURLY
    ;

type_reference
    : STAR? IDENTIFIER
    ;

number
    : DIGIT DIGIT* number_suffix?
    ;

number_suffix
    : IDENTIFIER
    ;

operator
    : OP_MINUS 
    | OP_PLUS 
    | STAR
    | OP_DIV 
    | OP_MOD 
    ;
