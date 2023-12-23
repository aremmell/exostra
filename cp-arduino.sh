#!/usr/bin/env zsh

_SOURCE_DIR="/Users/ryan/Documents/PlatformIO/Projects/exostra"
_TARGET_DIR="/Users/ryan/Documents/Arduino/exostra/main"

declare -a _src_files=(
	"${_SOURCE_DIR}/src/main.cpp"
	"${_SOURCE_DIR}/include/exostra.h"
)

declare -a _tgt_files=(
	"${_TARGET_DIR}/main.ino"
	"${_TARGET_DIR}/exostra.h"
)

_result=true
for i in {1..${#_src_files}}; do
	echo "${_src_files[$i]} -> ${_tgt_files[$i]}"
	cp "${_src_files[$i]}" "${_tgt_files[$i]}" || { _result=false }
done

if ! [ $_result = true ]; then
	echo "failed to copy all files!"
else
	echo "done"
fi
