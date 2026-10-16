# Highlight.js language definition for Capylang

This package adds Capylang syntax highlighting to
[Highlight.js](https://highlightjs.org/) as a third-party language definition.
Its lexical rules follow the ANTLR grammar in the parent directory.

## Install

### Workflow package

Download the `highlightjs-capylang-<commit>` artifact from a successful
`Package Highlight.js language` GitHub Actions run. Extract the artifact and
install the contained npm package archive:

```sh
npm install ./highlightjs-capylang-0.1.0.tgz highlight.js
```

### Source checkout

Install directly from the source tree during development:

```sh
npm install ./grammar/highlightjs highlight.js
```

## Use

Register the definition with a Highlight.js instance before highlighting:

```js
const hljs = require("highlight.js/lib/core");
const capylang = require("highlightjs-capylang");

hljs.registerLanguage("capylang", capylang);

const html = hljs.highlight("fn answer() -> u32 { 42u32 }", {
  language: "capylang"
}).value;
```

The registered language also has the `capy` alias. In browser integrations that
use registered Highlight.js languages, fenced code or `<code>` blocks can use
either `language-capylang` or `language-capy`.

## Development

```sh
cd grammar/highlightjs
npm ci
npm test
npm pack
```
