"use strict";

const assert = require("node:assert/strict");
const test = require("node:test");
const hljs = require("highlight.js/lib/core");
const capylang = require("../src/capylang");

hljs.registerLanguage("capylang", capylang);

test("registers Capylang and its capy alias", () => {
  assert.equal(hljs.getLanguage("capylang").name, "Capylang");
  assert.equal(hljs.getLanguage("capy").name, "Capylang");
});

test("highlights declarations, types, literals, and comments", () => {
  const source = `@export
fn answer(value: u32) -> u32 {
  // The answer
  let pointer: *u8 = 42u32;
  let text = "capy \\u{1f600}";
  let data = b"capy\\x21";
}`;
  const result = hljs.highlight(source, { language: "capylang" });

  assert.equal(result.illegal, false);
  assert.match(result.value, /hljs-meta[^>]*>@export/);
  assert.match(result.value, /hljs-keyword[^>]*>fn/);
  assert.match(result.value, /hljs-title function_[^>]*>answer/);
  assert.match(result.value, /hljs-type[^>]*>u32/);
  assert.match(result.value, /hljs-number[^>]*>42u32/);
  assert.match(result.value, /hljs-comment[^>]*>\/\/ The answer/);
  assert.match(result.value, /hljs-char escape_[^>]*>\\u\{1f600\}/);
  assert.match(result.value, /hljs-char escape_[^>]*>\\x21/);
});

test("does not highlight keywords inside identifiers", () => {
  const result = hljs.highlight(
    "let $if_value = gift(); let $123 = canvas::width();",
    {
      language: "capylang"
    }
  );

  assert.match(result.value, /hljs-keyword[^>]*>let/);
  assert.doesNotMatch(result.value, /hljs-keyword[^>]*>if/);
  assert.doesNotMatch(result.value, /hljs-number[^>]*>123/);
  assert.doesNotMatch(result.value, /hljs-type[^>]*>width/);
});
