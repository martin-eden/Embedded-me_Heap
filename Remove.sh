#!/bin/bash

# Uninstall library
arduino-cli lib uninstall me_Heap

# Uninstall dependencies
arduino-cli lib uninstall \
  me_Bits \
  me_MemorySegment \
  me_ManagedMemory \
  me_Console \
  me_InstallStandardStreams \
  me_UartSpeeds \
  me_BaseTypes
