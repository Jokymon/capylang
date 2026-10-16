# Capylang support for Vim

This Vim runtime plugin provides syntax highlighting and automatic filetype
detection for `*.capy` files. It also configures `//` comments and adds `.capy`
to Vim's filename suffix search.

## Install

Copy or link this directory to a Vim package directory. For example, on Linux
or macOS:

```sh
mkdir -p ~/.vim/pack/capylang/start
ln -s /path/to/capylang/grammar/vim ~/.vim/pack/capylang/start/capylang
```

On Windows, copy `grammar/vim` to:

```text
%USERPROFILE%\vimfiles\pack\capylang\start\capylang
```

Then ensure filetype plugins and syntax highlighting are enabled in `.vimrc`:

```vim
filetype plugin on
syntax enable
```

To try the plugin without installing it, start Vim with the directory added to
the runtime path:

```sh
vim --cmd "set runtimepath^=/path/to/capylang/grammar/vim" example.capy
```
