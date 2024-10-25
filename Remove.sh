#!/bin/bash

# Uninstall library
arduino-cli lib uninstall me_Heap

# Uninstall dependencies
arduino-cli lib uninstall \
  https://github.com/martin-eden/Embedded-me_Bits \
  https://github.com/martin-eden/Embedded-me_MemorySegment \
  https://github.com/martin-eden/Embedded-me_ManagedMemory \
  https://github.com/martin-eden/Embedded-me_Console \
  https://github.com/martin-eden/Embedded-me_InstallStandardStreams \
  https://github.com/martin-eden/Embedded-me_UartSpeeds \
  https://github.com/martin-eden/Embedded-me_BaseTypes
