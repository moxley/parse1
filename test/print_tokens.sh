#!/bin/sh

prog="11111222233 + 2 = abc
a = 1 + 2
"foo\nbar"
'joe\'s'
funcA()
while 1 == 1
end
func myFunc
"

echo "$prog"

echo "$prog" | ./bin/print_tokens
