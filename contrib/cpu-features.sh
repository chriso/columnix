#!/bin/bash

if [ "$(uname -s)" = Darwin ]; then
  echo $(sysctl -n machdep.cpu.features) $(sysctl -n machdep.cpu.leaf7_features) | tr . _
else
  cat /proc/cpuinfo | grep flags | head -1 | cut -d: -f2- | tr 'a-z' 'A-Z'
fi
