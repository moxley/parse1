#!/bin/sh

exp='1 + 3 * (5 - 1) / a
9 * 9
b = 2 + 3
c = 3 == 4 - 1
2 + 2 * 3 - 1
funcA(2, 3, 4==4)
while 1 == 1
  println("Foo")
end
"string1" + " string2"
'

echo "Expression: $exp"
echo "$exp" | ./bin/test_icode
