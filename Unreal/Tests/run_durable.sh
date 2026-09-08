#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${MAI_TEST_OUTPUT:-Unreal/Tests/.out}"
mkdir -p "$out"
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -I Unreal/MakeYourAI/Source/MakeYourAI/Public \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Rules/MaiDurableFile.cpp \
  Unreal/Tests/native/durable_main.cpp -o "$out/durable-tests"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
"$out/durable-tests" "$scratch"
"$out/durable-tests" "$scratch/restart" export
"$out/durable-tests" "$scratch/restart" resume
"$out/durable-tests" "$scratch/restart" recover
