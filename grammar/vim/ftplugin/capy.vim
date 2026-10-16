" Vim filetype settings for Capylang.
if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setlocal commentstring=//\ %s
setlocal comments=://
setlocal suffixesadd=.capy
setlocal iskeyword+=36

let b:undo_ftplugin = 'setlocal commentstring< comments< suffixesadd< iskeyword<'
