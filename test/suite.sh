#!/bin/sh
# balance suite: every stage, kid + pro bots, both modes. One JSON-ish line per run.
ROM=${1:-build/sneaky_kryptid.gbc}
STAGES=${2:-"1 2 3 4"}
for s in $STAGES; do
  python3 test/bot.py $ROM cadet kid 1 --stage $s 2>&1 | grep -v Warn &
  python3 test/bot.py $ROM cadet kid 2 --stage $s 2>&1 | grep -v Warn &
  wait
  python3 test/bot.py $ROM hunter pro 1 --stage $s 2>&1 | grep -v Warn &
  python3 test/bot.py $ROM hunter pro 2 --stage $s 2>&1 | grep -v Warn &
  wait
done
