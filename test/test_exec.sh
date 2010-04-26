#!/bin/sh

./bin/test_exec <<EOF
a = 1 + 3
b = funcA(5)
EOF
