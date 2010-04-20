#!/bin/sh

# Print expressions

./bin/print_exprs <<EOF
funcA()
funcB(funcC(1, 2), 3)
funcD()
EOF
