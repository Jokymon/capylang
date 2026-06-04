lexer grammar CapylangLexerGrammar;

ARROW : '->' ;
AT : '@' ;
BARBAR : '||' ;
AMPAMP : '&&' ;
COLON : ':' ;
DCOLON : '::' ;
DOT : '.' ;
NEQ : '!=' ;
EQEQ : '==' ;
EQ : '=' ;
LTE : '<=' ;
LT : '<' ;
GTE : '>=' ;
GT : '>' ;
SHL : '<<' ;
SHR : '>>' ;
COMMA : ',' ;
SEMI : ';' ;
STAR : '*' ;
DQUOTE : '"' ;
QUOTE : '\'' ;
LPAREN : '(' ;
RPAREN : ')' ;
LCURLY : '{' ;
RCURLY : '}' ;

KW_ALLOCATE : 'allocate';
KW_BREAK : 'break';
KW_ELSE : 'else';
KW_FN : 'fn';
KW_GLOBAL : 'global';
KW_IF : 'if';
KW_IMPORT : 'import';
KW_LET : 'let';
KW_MUT : 'mut';
KW_RECORD : 'record';
KW_RETURN : 'return';
KW_WHILE : 'while';

OP_MINUS : '-';
OP_PLUS : '+';
OP_DIV : '/';
OP_MOD : '%';
OP_CONV : 'as';

DIGIT : [0-9] ;
fragment HEX_DIGIT: [0-9a-fA-F];

fragment ID_START : [a-zA-Z_$] ;
fragment ID : ID_START | DIGIT ;

IDENTIFIER : ID_START (ID)* ;

fragment QUOTE_ESCAPE: '\\' ['"];
fragment UNICODE_ESCAPE:
    '\\u{' HEX_DIGIT HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? '}'
	;

STRING_LITERAL : DQUOTE ( ~["] | QUOTE_ESCAPE | UNICODE_ESCAPE )* DQUOTE ;

CHAR_LITERAL : QUOTE ( ~['] | QUOTE_ESCAPE | UNICODE_ESCAPE ) QUOTE ;

WS : [ \t\n\r\f]+ -> skip ;

LINE_COMMENT
	: '//' ~[\r\n]* -> channel(HIDDEN)
	;