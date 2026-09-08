#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
: "${MAI_QUICKJS_SOURCE:?Set to the pinned QuickJS source directory}"
: "${MAI_QUICKJS_LIBRARY:?Set to an actually compiled libqjs.a}"
out="${MAI_TEST_OUTPUT:-Unreal/Tests/.out}"
mkdir -p "$out"
node Unreal/Rules/build.mjs
node_modules/.bin/tsc -p Unreal/Rules/tsconfig.json
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -I Unreal/MakeYourAI/Source/MakeYourAI/Public -I "$MAI_QUICKJS_SOURCE" \
 Unreal/MakeYourAI/Source/MakeYourAI/Private/Rules/MaiRulesVM.cpp Unreal/Tests/rules_host.cpp "$MAI_QUICKJS_LIBRARY" -lm -ldl -lpthread -o "$out/rules_host"
MAI_RULES_HOST="$PWD/$out/rules_host" node Unreal/Tests/run_rules_parity.mjs
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -I Unreal/MakeYourAI/Source/MakeYourAI/Public \
 Unreal/MakeYourAI/Source/MakeYourAI/Private/Rules/MaiDurableFile.cpp Unreal/Tests/native/durable_main.cpp -o "$out/durable-tests"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
"$out/durable-tests" "$scratch"
