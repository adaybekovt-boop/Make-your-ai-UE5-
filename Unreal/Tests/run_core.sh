#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${MAI_TEST_OUTPUT:-Unreal/Tests/.out}"
mkdir -p "$out"
${CXX:-g++} -std=c++17 -Wall -Wextra -Werror -Wno-misleading-indentation -pedantic \
  -I Unreal/MakeYourAI/Source/MakeYourAI/Public \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiCatalog.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiSimulation.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Core/MaiCodec.cpp \
  Unreal/MakeYourAI/Source/MakeYourAI/Private/Campaign/MaiCampaign*.cpp \
  Unreal/Tests/native/main.cpp Unreal/Tests/native/campaign_tests.cpp -o "$out/core-tests"
"$out/core-tests"
"$out/core-tests" --fixtures > "$out/fixtures.json"
