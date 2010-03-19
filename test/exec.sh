#!/bin/sh

`dirname $0`/../bin/exec<<EOF
print 5
print 1 + 1
EOF

