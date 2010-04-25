#!/bin/sh

exp='1 + 3 * (5 - 1) / a
9 * 9
b = 2 + 3
c = 3 == 4 - 1
'
echo "Expression: $exp"
echo "$exp" | ./bin/test_icode
