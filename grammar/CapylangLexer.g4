lexer grammar CapyLangLexer;

ARROW : '->' ;
DCOLON : '::' ;
EQ : '=' ;
COMMA : ',' ;
SEMI : ';' ;
LPAREN : '(' ;
RPAREN : ')' ;
LCURLY : '{' ;
RCURLY : '}' ;

KW_FN: 'fn';
KW_IMPORT: 'import';

OP_MINUS: '-';
OP_PLUS: '+';
OP_MULT: '*';
OP_DIV: '/';
OP_MOD: '%';
OP_CONV: 'as';

DIGIT: [0-9] ;

ID_START: [a-zA-Z_$] ;
ID: ID_START | DIGIT;

IDENTIFIER: ID_START ID* ;

WS: [ \t\n\r\f]+ -> skip ;

LINE_COMMENT
	: '//' ~[\r\n]* -> channel(HIDDEN)
	;