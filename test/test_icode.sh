#!/bin/sh

exp='1 + 3 * (5 - 1) / b'
echo "Expression: $exp"
echo "$exp" | ./bin/test_icode
