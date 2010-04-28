#!/bin/sh

#./bin/test_exec <<EOF
#if 1 == 2
#  a = 1
#else if 2 == 2
#  a = 2
#else
#  a = 3
#end
#c = funcA(10 + 5 * 2)
#b = 9
#EOF

./bin/test_exec <<EOF
if 1 == 2
  a = 1
end
b = 9
EOF
