#!/bin/bash
# As probas de Barullo Verb na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows, linux ou sanitizadores, coa compilación en build/
set -eo pipefail
SO="$1"
EXE=""; [ "$SO" = windows ] && EXE=".exe"
CONF=Release; [ "$SO" = sanitizadores ] && CONF=RelWithDebInfo
RT="build/BVRenderTest_artefacts/$CONF/BVRenderTest$EXE"

echo "::group::Son (os tipos, os tempos, o predelay, conxelar, shimmer, os presets e o estado)"
"$RT" proba.wav
echo "::endgroup::"
