#!/bin/sh

./bin/test_exec <<EOF && echo "Expected: a=4, b=5, c=12"
a = 1 + 3
b = funcA(5)
c = funcA(funcA(6+6))
EOF


#echo "println('hello')" | ./bin/test_exec
