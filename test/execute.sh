#!/bin/sh

`dirname $0`/../bin/execute<<EOF
hello()
print(5)
EOF
