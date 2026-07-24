#pragma once

const char INST_BLOCK = '\x02';
const char INST_LOOP = '\x03';
const char INST_IF = '\x04';
const char INST_ELSE = '\x05';
const char INST_TERMINATOR = '\x0b';
const char INST_BR = '\x0c';
const char INST_BR_IF = '\x0d';

const char INST_UNREACHABLE = '\x00';
const char INST_NOP = '\x01';
const char INST_DROP = '\x1a';
const char INST_SELECT = '\x1b';

const char INST_CALL = '\x10';
const char INST_RET = '\x0f';

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

const char INST_MEMORY_SIZE = '\x3f';
const char INST_MEMORY_GROW = '\x40';

const char INST_I32_CONST = '\x41';
const char INST_I64_CONST = '\x42';
const char INST_F32_CONST = '\x43';
const char INST_F64_CONST = '\x44';
const char INST_I32_EQZ = '\x45';
const char INST_I32_EQ = '\x46';
const char INST_I32_NE = '\x47';
const char INST_I32_LT_S = '\x48';
const char INST_I32_LT_U = '\x49';
const char INST_I32_GT_S = '\x4a';
const char INST_I32_GT_U = '\x4b';
const char INST_I32_LE_S = '\x4c';
const char INST_I32_LE_U = '\x4d';
const char INST_I32_GE_S = '\x4e';
const char INST_I32_GE_U = '\x4f';

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
const char INST_I32_AND = '\x71';
const char INST_I32_OR = '\x72';
const char INST_I32_XOR = '\x73';
const char INST_I32_SHL = '\x74';
const char INST_I32_SHR_S = '\x75';
const char INST_I32_SHR_U = '\x76';

// Multibyte instructions starting with FC
const char INST_FC = '\xfc';

const char INST_FC_MEMORY_FILL = '\x0b';