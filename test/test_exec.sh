#!/bin/sh

./bin/test_exec <<EOF
a = 1 + 3
b = funcA(5)
c = funcA(funcA(6+6))
EOF
