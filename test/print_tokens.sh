#!/bin/sh

prog="11111222233 + 2 = abc
a = 1 + 2
"foo\nbar"
'joe\'s'
func()
while 1 == 1
end
"

echo "$prog"

echo "$prog" | ./bin/print_tokens
