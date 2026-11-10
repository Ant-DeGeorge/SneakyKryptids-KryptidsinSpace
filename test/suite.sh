#!/bin/sh
# balance suite: every stage, kid + pro bots, all three modes. One JSON-ish line per run.
ROM=${1:-build/sneaky_kryptid.gbc}
STAGES=${2:-"1 2 3 4 5"}
for s in $STAGES; do
  [ "$s" = 5 ] && continue
  python3 test/bot.py $ROM cadet kid 1 --stage $s 2>&1 | grep -v Warn &
  python3 test/bot.py $ROM cadet kid 2 --stage $s 2>&1 | grep -v Warn &
  wait
  python3 test/bot.py $ROM pilot kid 1 --stage $s 2>&1 | grep -v Warn &
  python3 test/bot.py $ROM pilot pro 1 --stage $s 2>&1 | grep -v Warn &
  wait
  python3 test/bot.py $ROM hunter pro 1 --stage $s 2>&1 | grep -v Warn &
  python3 test/bot.py $ROM hunter pro 2 --stage $s 2>&1 | grep -v Warn &
  wait
done
# mission 5 (gun-cam stage) has its own bot
case " $STAGES " in *" 5 "*)
  python3 test/bot_cockpit.py $ROM cadet kid 1 2>&1 | grep -v Warn &
  python3 test/bot_cockpit.py $ROM cadet kid 2 2>&1 | grep -v Warn &
  wait
  python3 test/bot_cockpit.py $ROM pilot kid 1 2>&1 | grep -v Warn &
  python3 test/bot_cockpit.py $ROM pilot pro 1 2>&1 | grep -v Warn &
  wait
  python3 test/bot_cockpit.py $ROM hunter pro 1 2>&1 | grep -v Warn &
  python3 test/bot_cockpit.py $ROM hunter pro 3 2>&1 | grep -v Warn &
  wait ;;
esac
