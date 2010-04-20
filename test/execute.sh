#!/bin/sh

`dirname $0`/../bin/execute<<EOF
hello()
add(1, 2)
println(5)
println(add(1, 2))
EOF

