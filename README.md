# FwUpdateLibs
This repository contains generic and configurable libraries (+custom tools) for achieving a reliable remote micro controller firmware update.
These libraries are used in the related [reliable_fw_update](https://github.com/mp-commits/reliable_fw_update) repository.

## crc
Generic library for a slow CRC32 function.

## ed25519
CMake wrapper for submodules/ed25519.

## fragmentstore
Generic configurable storage library to store firmware fragments.

## hexfile
C++ library for parsing IntelHex files from/to fstreams.

## keyfile
C++ library for parsing OpenSSH keypairs and a tool for generating C headers from said keyfiles.

## niram
No init RAM area library used in reliable_fw_update repo components.

## updateclient
Testing client using UDP to connect to implementation in reliable_fw_update repo.

## updateserver
Generic and configurable server for protocol implemented in reliable_fw_update repo

## w259xx
Wrapper and interface library for submodules/w25qxx
