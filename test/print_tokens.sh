#!/bin/sh

./bin/print_tokens <<EOF
11111222233 + 2 = abc
a = 1 + 2
"foo\nbar"
'joe\'s'
func()
EOF
