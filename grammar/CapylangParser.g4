parser grammar CapyLang;

module: function_import* function_definition*;

function_import: KW_IMPORT IDENTIFIER DCOLON IDENTIFIER LPAREN RPAREN;

function_definition: KW_FN IDENTIFIER LPAREN RPAREN 
    ( ARROW IDENTIFIER )?
    LCURLY
        expression
    RCURLY ;

expression:
    LPAREN expression RPAREN ( OP_CONV type_reference )? 
    | primary ( operator expression )* ;

primary: IDENTIFIER ( LPAREN expression RPAREN )?
    | number ;

type_reference: IDENTIFIER ;

number: DIGIT DIGIT* number_suffix? ;

number_suffix: IDENTIFIER ;

operator: OP_MINUS 
    | OP_PLUS 
    | OP_MULT
    | OP_DIV 
    | OP_MOD 
    | OP_CONV ;
