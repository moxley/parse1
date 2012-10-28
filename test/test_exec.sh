#!/bin/sh

expr='
a = 1 + 3
a = a + 1
i = 0
while i < 10
  println("While(i=" + i + ")")
  i = i + 1
end
b = funcA(5)
c = funcA(funcA(6+6))
d = "Hello" + ", world!"
e = "She is " + 6 + " years old."
'

expr='func myFunc()
  println("This is myFunc()")
end
myFunc()
println("This is the end")
'

echo "$expr"
echo "$expr" | ./bin/test_exec 3
