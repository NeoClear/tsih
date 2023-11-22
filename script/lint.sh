#!/bin/zsh

ROOT=$(git rev-parse --show-toplevel)

if [[ $? -ne 0 ]]; then
	echo "Git repo not found"
	exit 1
fi

clang-format -i -style=file $ROOT/**/*.h $ROOT/**/*.cc
buildifier $ROOT/**/BUILD $ROOT/WORKSPACE
shfmt -l -w $ROOT/**/*.sh
