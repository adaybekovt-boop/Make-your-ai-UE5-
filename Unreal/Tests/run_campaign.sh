#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${MAI_TEST_OUTPUT:-Unreal/Tests/.out}"
mkdir -p "$out"
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -pedantic -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I Unreal/MakeYourAI/Source/MakeYourAI/Public \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiCatalog.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiSimulation.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiCodec.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Campaign/MaiCampaign*.cpp \
  Unreal/Tests/native/campaign_main.cpp -o "$out/campaign-tests"
"$out/campaign-tests" | tee "$out/campaign.log"
scratch="$(mktemp -d)"
trap 'rm -rf "$scratch"' EXIT
"$out/campaign-tests" --export "$scratch"
"$out/campaign-tests" --resume "$scratch/campaign.save" > "$scratch/resumed.save"
cmp "$scratch/expected.save" "$scratch/resumed.save"
echo 'CROSS_PROCESS_RESTART_PASS: saved campaign continues identically in a new native process'
