#!/bin/sh

./bin/test_exec <<EOF
if 1 == 1
  a = 1
else if 2 == 2
  a = 2
else
  a = 3
end
EOF
