#!/bin/sh

`dirname $0`/../bin/execute<<EOF
hello()
add(1, 2)
print(5)
EOF
