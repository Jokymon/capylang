#pragma once

const char INST_TERMINATOR = '\x0b';

const char INST_UNREACHABLE = '\x00';
const char INST_NOP = '\x01';
const char INST_DROP = '\x1a';
const char INST_SELECT = '\x1b';

const char INST_CALL = '\x10';

const char INST_LOCAL_GET = '\x20';
const char INST_LOCAL_SET = '\x21';
const char INST_LOCAL_TEE = '\x22';
const char INST_GLOBAL_GET = '\x23';
const char INST_GLOBAL_SET = '\x24';

const char INST_I32_LOAD = '\x28';
const char INST_I32_LOAD8_S = '\x2c';
const char INST_I32_LOAD8_U = '\x2d';
const char INST_I32_STORE = '\x36';
const char INST_I32_STORE8 = '\x3a';

const char INST_I32_CONST = '\x41';
const char INST_I64_CONST = '\x42';
const char INST_F32_CONST = '\x43';
const char INST_F64_CONST = '\x44';
const char INST_I32_EQZ = '\x45';
const char INST_I32_EQ = '\x46';
const char INST_I32_NE = '\x47';

const char INST_I32_CLZ = '\x67';
const char INST_I32_CTZ = '\x68';
const char INST_I32_POPCNT = '\x69';
const char INST_I32_ADD = '\x6a';
const char INST_I32_SUB = '\x6b';
const char INST_I32_MUL = '\x6c';
const char INST_I32_DIV_S = '\x6d';
const char INST_I32_DIV_U = '\x6e';
const char INST_I32_REM_S = '\x6f';
const char INST_I32_REM_U = '\x70';