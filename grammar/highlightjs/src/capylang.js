"use strict";

/**
 * Capylang language definition for Highlight.js.
 *
 * Keep the lexical rules aligned with ../CapylangLexerGrammar.g4.
 *
 * @param {import("highlight.js").HLJSApi} hljs
 * @returns {import("highlight.js").Language}
 */
module.exports = function capylang(hljs) {
  const IDENTIFIER = /[A-Za-z_$][A-Za-z0-9_$]*/;
  const IDENTIFIER_BOUNDARY = /(?:^|[^A-Za-z0-9_$])/;

  const UNICODE_ESCAPE = {
    scope: "char.escape",
    match: /\\(?:['"]|u\{[0-9A-Fa-f]{1,6}\})/,
    relevance: 0
  };

  const BLOB_ESCAPE = {
    scope: "char.escape",
    match: /\\(?:['"]|x[0-9A-Fa-f]{2})/,
    relevance: 0
  };

  const BLOB = {
    scope: "string",
    begin: /b"/,
    end: /"/,
    contains: [BLOB_ESCAPE],
    relevance: 1
  };

  const STRING = {
    scope: "string",
    begin: /"/,
    end: /"/,
    contains: [UNICODE_ESCAPE],
    relevance: 0
  };

  const CHARACTER = {
    scope: "string",
    begin: /'/,
    end: /'/,
    contains: [UNICODE_ESCAPE],
    relevance: 0
  };

  const FUNCTION_DECLARATION = {
    match: [IDENTIFIER_BOUNDARY, /fn/, /\s+/, IDENTIFIER],
    beginScope: {
      2: "keyword",
      4: "title.function"
    },
    relevance: 2
  };

  const RECORD_DECLARATION = {
    match: [IDENTIFIER_BOUNDARY, /record/, /\s+/, IDENTIFIER],
    beginScope: {
      2: "keyword",
      4: "title.class"
    },
    relevance: 2
  };

  const ALLOCATION = {
    match: [IDENTIFIER_BOUNDARY, /allocate/, /\s+/, IDENTIFIER],
    beginScope: {
      2: "keyword",
      4: "title.class"
    },
    relevance: 1
  };

  const TYPE_REFERENCE = {
    match: [/(?:->|:)/, /\s*/, /\*?/, /\s*/, IDENTIFIER],
    beginScope: {
      1: "operator",
      3: "operator",
      5: "type"
    },
    relevance: 0
  };

  const CAST_TYPE = {
    match: [IDENTIFIER_BOUNDARY, /as/, /\s+/, /\*?/, /\s*/, IDENTIFIER],
    beginScope: {
      2: "keyword",
      4: "operator",
      6: "type"
    },
    relevance: 0
  };

  return {
    name: "Capylang",
    aliases: ["capy"],
    keywords: {
      $pattern: IDENTIFIER,
      keyword:
        "allocate break else fn global if import include let mut record return while as"
    },
    contains: [
      hljs.COMMENT(/\/\//, /$/, { relevance: 0 }),
      BLOB,
      STRING,
      CHARACTER,
      FUNCTION_DECLARATION,
      RECORD_DECLARATION,
      ALLOCATION,
      CAST_TYPE,
      {
        scope: "operator",
        match: /::/,
        relevance: 0
      },
      TYPE_REFERENCE,
      {
        scope: "meta",
        match: /@[A-Za-z_$][A-Za-z0-9_$]*/,
        relevance: 1
      },
      {
        match: [
          IDENTIFIER_BOUNDARY,
          /\d+(?:[A-Za-z_$][A-Za-z0-9_$]*)?(?![A-Za-z0-9_$])/
        ],
        beginScope: {
          2: "number"
        },
        relevance: 0
      },
      {
        scope: "operator",
        match: /->|\|\||&&|!=|==|<=|>=|<<|>>|[-+/%*=<>]/,
        relevance: 0
      }
    ]
  };
};
