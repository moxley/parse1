#!/bin/sh

exp='1 + 3 * (5 - 1) / a
9 * 9
b = 2 + 3
c = 3 == 4 - 1
2 + 2 * 3 - 1
funcA()
'

exp='funcA(7, 8, 9)'
echo "Expression: $exp"
echo "$exp" | ./bin/test_icode
