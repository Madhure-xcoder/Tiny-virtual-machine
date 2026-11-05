
# Integrated VM

SET i 1
SET sum 0
SET limit 5

LOOP:
GT check i limit
JNZ check STACK

ADD sum sum i
ADD i i 1
JMP LOOP

STACK:
PUSH sum
PUSH 2
MUL
PRINT
HALT
