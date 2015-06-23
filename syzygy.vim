" Vim syntax file for Syzygy.
"
" Copy this file to $VIMRUNTIME/syntax/syzygy.vim
" (e.g., ~/.vim/syntax/syzygy.vim or /usr/share/vim/vim74/syntax/syzygy.vim).
"
" Ensure that "set modeline" is true, e.g. by adding that line to ~/.vimrc.
"
" To use this with a .xml file, in that file include a line:
" // vim: filetype=syzygy

:so $VIMRUNTIME/syntax/xml.vim

" Comments in green.
:syntax region szgComment start=/<comment>/ end=/<\/comment>/
:highlight szgComment ctermfg=lightgreen guifg=lightgreen

" todo: within an <assign> ... </assign> block,
" four colors for the four entries in each line.
" :syntax region szgAssign start=/<assign>/ end=/<\/assign>/
" ...
