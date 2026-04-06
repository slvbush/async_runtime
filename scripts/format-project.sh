#!/bin/bash

cd "$(dirname "$0")/.." || exit 1

DIRS_TO_FORMAT="lib test"

find $DIRS_TO_FORMAT -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | while read -r file; do
    echo "Formatting: $file"
    clang-format -i --style=file "$file"
done

echo "done."
