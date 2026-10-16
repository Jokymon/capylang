# Capylang support for Vim

This Vim runtime plugin provides syntax highlighting and automatic filetype
detection for `*.capy` files. It also configures `//` comments and adds `.capy`
to Vim's filename suffix search.

## Install

### Packaged release

Download `capylang-vim.zip` or `capylang-vim.tar.gz` from a GitHub release. The
archives contain a native Vim package named `capylang`; extract that directory
under the appropriate `pack/capylang/start` directory:

- Vim on Linux or macOS: `~/.vim/pack/capylang/start/`
- Vim on Windows: `%USERPROFILE%\vimfiles\pack\capylang\start\`
- Neovim on Linux or macOS: `~/.local/share/nvim/site/pack/capylang/start/`
- Neovim on Windows: `%LOCALAPPDATA%\nvim-data\site\pack\capylang\start\`

Packages produced for every relevant commit can also be downloaded from the
`Package Vim plugin` workflow run's Artifacts section. The workflow artifact
contains both archive formats.

### Source checkout

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
