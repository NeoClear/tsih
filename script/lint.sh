#!/bin/zsh

ROOT=$(git rev-parse --show-toplevel)

if [[ $? -ne 0 ]]; then
	echo "Git repo not found"
	exit 1
fi

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	find $ROOT -name "*.h" -or -name "*.cc" -or -name "*.sh" -or -name "BUILD" | xargs sed -i -e '$a\'
elif [[ "$OSTYPE" == "darwin"* ]]; then
	for file in $(find $ROOT -name "*.h" -or -name "*.cc" -or -name "*.sh" -or -name "BUILD"); do
		echo >>$file
	done
fi

clang-format -i -style=file $ROOT/**/*.h $ROOT/**/*.cc
buildifier $ROOT/**/BUILD $ROOT/WORKSPACE
shfmt -l -w $ROOT/**/*.sh
