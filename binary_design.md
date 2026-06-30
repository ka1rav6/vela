# BINARY BYTECODE DESIGN 

### Magic Number
"RULE" in ascii
 > 52 4C 45 42
(offset : 0 - 4)

### Version : uint32
(offset : 4 - 8)

### Number of instructions : uint32

(offset : 8- 12)

### Operation Design

From bytecode.h:
// OpCode enum order (implicit 0,1,2,3,4,5):
OP_PUSH_FACT = 0
OP_PUSH_CMP  = 1
OP_AND       = 2
OP_OR        = 3
OP_NOT       = 4
OP_HALT      = 5

// CompareOp enum order:
OP_LT = 0   // <
OP_LE = 1   // <=
OP_GT = 2   // >
OP_GE = 3   // >=
OP_EQ = 4   // ==
OP_NE = 5   // !=

EXAMPLE : Instruction 0:
OP_PUSH_CMP
Offset      Bytes       Field
12          01          OP_PUSH_CMP (enum val = 0)



## Final Bytes and what they represent:

- Header: 
    1. Magic Number
    2. Version Number
    3. Instruction count

- Body:
    1. Opcode
    2. Operation
    3. Name
    4. Value (number)
