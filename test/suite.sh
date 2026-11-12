#!/bin/sh
# balance suite: every stage, kid + pro bots, all three modes. One JSON-ish line per run.
ROM=${1:-build/sneaky_kryptid.gbc}
STAGES=${2:-"1 2 3 4 5 6"}   # mission 7 is its own ROM (tree_line.gbc); bot_ground.py needs a 7-mission build
for s in $STAGES; do
  [ "$s" = 5 ] && continue
  [ "$s" = 6 ] && continue
  [ "$s" = 7 ] && continue
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
# mission 6 (escape run: no shooting) has its own bot
case " $STAGES " in *" 6 "*)
  python3 test/bot_escape.py $ROM cadet kid 1 2>&1 | grep -v Warn &
  python3 test/bot_escape.py $ROM cadet kid 2 2>&1 | grep -v Warn &
  wait
  python3 test/bot_escape.py $ROM pilot kid 1 2>&1 | grep -v Warn &
  python3 test/bot_escape.py $ROM pilot pro 1 2>&1 | grep -v Warn &
  wait
  python3 test/bot_escape.py $ROM hunter pro 1 2>&1 | grep -v Warn &
  python3 test/bot_escape.py $ROM hunter pro 2 2>&1 | grep -v Warn &
  wait ;;
esac
# mission 7 (the tree line: Graul on foot) has its own bot
case " $STAGES " in *" 7 "*)
  python3 test/bot_ground.py $ROM cadet kid 1 2>&1 | grep -v Warn &
  python3 test/bot_ground.py $ROM cadet kid 2 2>&1 | grep -v Warn &
  wait
  python3 test/bot_ground.py $ROM pilot kid 1 2>&1 | grep -v Warn &
  python3 test/bot_ground.py $ROM pilot pro 1 2>&1 | grep -v Warn &
  wait
  python3 test/bot_ground.py $ROM hunter pro 1 2>&1 | grep -v Warn &
  python3 test/bot_ground.py $ROM hunter pro 2 2>&1 | grep -v Warn &
  wait ;;
esac
