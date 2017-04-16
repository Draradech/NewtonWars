#!/bin/sh

at now <<EOF
while true ; do
    sleep 2
    test ! -c "${DEVNAME}" && exit 1
    nc 127.0.0.1 3490 < "${DEVNAME}" > "${DEVNAME}"
    test ! -c "${DEVNAME}" && exit 1
done
EOF

