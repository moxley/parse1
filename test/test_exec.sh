#!/bin/sh

expr='
a = 1 + 3
b = funcA(5)
c = funcA(funcA(6+6))
d = "Hello" + ", world!"
e = "She is " + 6 + " years old."
'
echo "$expr"
echo "$expr" | ./bin/test_exec
