#!/bin/bash
# Regenerate the patched studio card from the pristine x1 export in orig/.
# Safe to re-run: always starts from orig/, so it never stacks patches.
set -e
cd "$(dirname "$0")"
D=../../src/studio_card
cp orig/studio_card_data.h orig/studio_card_gfx.h orig/studio_card_counts.h $D/
python3 mod_card.py
