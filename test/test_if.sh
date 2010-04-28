#!/bin/sh

./bin/test_exec <<EOF
if 1 == 2
  a = 1
else if 2 == 2
  a = 2
else
  a = 3
end
c = funcA(10 + 5 * 2)
b = 9
EOF
echo "Expected: a=2, c=20, b=9"

./bin/test_exec <<EOF
a = 0
if 1 == 2
  a = 1
end
b = 9
EOF
echo "Expected: a=0, b=9"

./bin/test_exec <<EOF
if 1 == 1
  a = 1
else
  a = 3
end
b = 9
EOF
echo "Expected: a=1, b=9"
