" Vim syntax file
" Language: Capylang
" Source:   grammar/CapylangLexerGrammar.g4

if exists('b:current_syntax')
  finish
endif

syn case match

" Literals. Keep these rules aligned with CapylangLexerGrammar.g4.
syn match capyEscape /\\\%(['"]\|x\x\{2}\|u{\x\{1,6}}\)/ contained
syn region capyBlob start=/\<b"/ skip=/\\"/ end=/"/ contains=capyBlobEscape
syn match capyBlobEscape /\\\%(['"]\|x\x\{2}\)/ contained
syn region capyString start=/"/ skip=/\\"/ end=/"/ contains=capyEscape,@Spell
syn region capyCharacter start=/'/ skip=/\\'/ end=/'/ contains=capyEscape
syn match capyNumber /\<\d\+\%([A-Za-z_$][A-Za-z0-9_$]*\)\?\>/

" Keywords and declarations
syn keyword capyStatement allocate global import include
syn keyword capyConditional if else
syn keyword capyRepeat while
syn keyword capyControl break return
syn keyword capyFunctionKeyword fn nextgroup=capyFunctionDecl skipwhite
syn keyword capyDeclaration let
syn keyword capyRecordKeyword record nextgroup=capyRecordDecl skipwhite
syn keyword capyStorageModifier mut
syn keyword capyOperator as

" Names whose role is unambiguous from the parser grammar.
syn match capyAttribute /@[A-Za-z_$][A-Za-z0-9_$]*/
syn match capyFunctionDecl /[A-Za-z_$][A-Za-z0-9_$]*/ contained
syn match capyRecordDecl /[A-Za-z_$][A-Za-z0-9_$]*/ contained

" Operators
syn match capyOperator /->\|::\|!=\|==\|<=\|>=\|<<\|>>\|&&/
syn match capyOperator /||/
syn match capyOperator /[-+\/%*=<>]/

" Define comments after operators so // wins at the same position.
syn keyword capyTodo TODO FIXME XXX NOTE contained
syn match capyComment "//.*$" contains=capyTodo,@Spell

hi def link capyTodo Todo
hi def link capyComment Comment
hi def link capyEscape SpecialChar
hi def link capyBlobEscape SpecialChar
hi def link capyBlob String
hi def link capyString String
hi def link capyCharacter Character
hi def link capyNumber Number
hi def link capyStatement Statement
hi def link capyConditional Conditional
hi def link capyRepeat Repeat
hi def link capyControl Statement
hi def link capyFunctionKeyword Keyword
hi def link capyDeclaration Keyword
hi def link capyRecordKeyword Keyword
hi def link capyStorageModifier StorageClass
hi def link capyOperator Operator
hi def link capyAttribute PreProc
hi def link capyFunctionDecl Function
hi def link capyRecordDecl Type

let b:current_syntax = 'capy'
