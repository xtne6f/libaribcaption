#!/bin/bash

# Place DRCS bitmap files output by TVTest in the "DRCS" directory.
# If matching hash values ​​already exist in "b24_drcs_conv.cpp", grep those lines.

./hash_drcs_bmp.exe "DRCS/${1^^}.bmp" | tr -d '\r' | while read hash; do
  if [ "${hash^^}" != "${1^^}" ]; then
    grep -n "$hash" ../../src/decoder/b24_drcs_conv.cpp
  fi
done
