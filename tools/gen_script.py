#!/usr/bin/env python3
"""Story script: crew names, per-stage briefings/debriefs and in-flight comms.

Cutscene lines wrap at 12 columns x 6 rows (beside a 48x48 portrait).
In-flight comm lines wrap at 15 columns x 3 rows (beside a 32x32 portrait).
Emits src/script_data.c (ROM0: comms), src/script_scenes.c (bank 7: cutscenes) and src/script_ids.h (CM* ids for stage logic).
"""
import os, textwrap

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src")

# ---- crew names (one place to change them)
NAMES = {
    'cap':    "YETI",
    'static': "STATIC",
    'forge':  "FORGE",
    'doc':    "SUTURE",
    'vex':    "ZANNAH",
    'graul':  "GRAUL",
}
CREW = ['cap', 'static', 'forge', 'doc', 'vex', 'graul']   # portrait order (graul: cutscenes only, art from gen_escape.py)

# scene ids (see scenes.c load_scene)
SC_SHIP, SC_STATION, SC_KRAKEN, SC_TBIRD, SC_JDEVIL, SC_STORM, SC_BARRENS, SC_WENDIGO, SC_FROST, SC_FLAT, SC_CRASH = range(11)

INTRO = [
  [ # 1 Rimeveil Lane
    (SC_STATION, 'vex',    0, "NEW CONTRACT. SOMETHING IS EATING ICE HAULERS."),
    (SC_STATION, 'cap',    0, "WHAT'S IT PAY?"),
    (SC_STATION, 'vex',    1, "TRIPLE IF WE BRING IT IN ALIVE. AND WE WILL."),
    (SC_STATION, 'doc',    0, "SIX HAULERS. ZERO DISTRESS CALLS. IT'S SMART."),
    (SC_SHIP,    'forge',  1, "RUNE NET IS LOADED. FLY CLEAN, SCORE BIG, AND IT'S OURS."),
    (SC_SHIP,    'static', 0, "SHOOT THEIR BULLETS, TOO. OUR ROUNDS EAT THEM."),
    (SC_SHIP,    'cap',    1, "SNEAKY KRYPTID. LET'S GO TO WORK."),
  ],
  [ # 2 Stormwall Nebula
    (SC_STORM,   'vex',    0, "RELAY GUILD. THEIR BEACONS KEEP GOING DARK."),
    (SC_STORM,   'static', 0, "SOMETHING IN THE STORM IS EATING THE LIGHTNING."),
    (SC_TBIRD,   'doc',    1, "A THUNDERBIRD. WINGSPAN OF A HAULER. GORGEOUS."),
    (SC_TBIRD,   'cap',    0, "SUTURE. FOCUS."),
    (SC_TBIRD,   'doc',    0, "GORGEOUS AND DEADLY. FOCUSED."),
    (SC_SHIP,    'forge',  1, "HULL IS RUNE-WARDED. LIGHTNING SLIDES OFF. MOSTLY."),
    (SC_SHIP,    'cap',    1, "MOSTLY IS GOOD ENOUGH. PUNCH IT."),
  ],
  [ # 3 Barrens Belt
    (SC_BARRENS, 'vex',    0, "BARRENS SETTLERS. SOMETHING SCREAMS AT NIGHT."),
    (SC_BARRENS, 'doc',    1, "THEY SLEEP WITH EVERY LIGHT ON. CAN'T BLAME THEM."),
    (SC_JDEVIL,  'static', 0, "HORSE SKULL. BAT WINGS. KANGAROO LEGS."),
    (SC_JDEVIL,  'cap',    0, "THAT'S NOT A MONSTER. THAT'S A SPARE PARTS BIN."),
    (SC_JDEVIL,  'doc',    1, "THE JERSEY DEVIL. AND IT LAYS EGGS. DON'T POP THEM."),
    (SC_SHIP,    'forge',  1, "COLD IRON ROUNDS. DEVILS HATE IRON."),
    (SC_SHIP,    'vex',    1, "AND I HEAR NUBS IS OUT HERE. HE STILL OWES ME."),
  ],
  [ # 4 Frostbite Moon
    (SC_FROST,   'vex',    1, "THE MINERS' GUILD. BIGGEST PAYOUT YET."),
    (SC_FROST,   'static', 0, "WHOLE MINING CREW, GONE. JUST A HOWL ON THE COMMS."),
    (SC_WENDIGO, 'doc',    0, "ANTLERS. BONES. ALWAYS HUNGRY. A WENDIGO."),
    (SC_WENDIGO, 'cap',    0, "HUNGRY FOR WHAT?"),
    (SC_WENDIGO, 'doc',    0, "YOU DON'T WANT ME TO ANSWER THAT."),
    (SC_SHIP,    'forge',  1, "FIRE RUNES ON THE CANNONS. IT HATES HEAT."),
    (SC_SHIP,    'cap',    1, "BUNDLE UP, CREW. WE'RE GOING HUNTING."),
  ],
  [ # 5 Flatwoods Drift (gun-cam mission)
    (SC_SHIP,    'static', 0, "THE FLATWOODS SIGNAL. IT'S COMING FROM A SHIP."),
    (SC_FLAT,    'doc',    0, "TEN FEET TALL. EYES LIKE LAMPS. FLATWOODS MONSTER."),
    (SC_FLAT,    'vex',    1, "NO CLIENT ON THIS ONE. THE BOUNTY BOARD POSTED IT ITSELF."),
    (SC_FLAT,    'cap',    0, "THAT'S NOT OMINOUS AT ALL."),
    (SC_SHIP,    'static', 0, "ITS SAUCERS JAM OUR HELM. I CAN HOLD HER STEADY. THAT'S ALL."),
    (SC_SHIP,    'forge',  1, "THEN USE THE GUN CAM. YOU AIM, SHE SHOOTS."),
    (SC_SHIP,    'cap',    1, "FINE. STATIC FLIES. I'LL TAKE THE GUNS."),
  ],
  [ # 6 Escape Run (no guns: dodge, graze, dive)
    (SC_SHIP,    'vex',    1, "FIVE CONTRACTS CLOSED. NEXT STOP, PAYDAY."),
    (SC_SHIP,    'static', 0, "CONTACT. BIG ONE. IT'S THE SCRAPJAW."),
    (SC_SHIP,    'cap',    0, "NUBS. AND THIS TIME HE BROUGHT THE WHOLE GANG."),
    (SC_SHIP,    'forge',  0, "EMP! CANNONS ARE DEAD. RUNES ARE DEAD."),
    (SC_SHIP,    'doc',    0, "PLANET BELOW US. THICK AIR. THAT HULK CAN'T FOLLOW."),
    (SC_SHIP,    'static', 1, "NO GUNS, SO FLY CLOSE. NEAR MISSES CHARGE THE BOOST."),
    (SC_SHIP,    'cap',    1, "THEN WE RUN. NOSE DOWN. DIVE, DIVE, DIVE!"),
  ],
  [ # 7 The Tree Line (Graul on foot, hold the wreck, the watcher)
    (SC_CRASH,   'forge',  0, "GUNS ARE SLAG. I NEED PARTS AND I NEED A DAY."),
    (SC_CRASH,   'static', 0, "NUBS DROPPED GROUND CREWS. THEY'RE SWEEPING FOR US."),
    (SC_CRASH,   'doc',    0, "AND THE WOODS ARE FULL OF THINGS WITH TEETH."),
    (SC_CRASH,   'graul',  1, "GOOD. I WAS GETTING BORED IN THAT POD."),
    (SC_CRASH,   'cap',    0, "CLEAR THE TREE LINE, WOLF. THEN GET BACK HERE."),
    (SC_CRASH,   'graul',  0, "SOMETHING OUT THERE SMELLS LIKE YOUR CARGO HOLD."),
    (SC_CRASH,   'cap',    1, "THEN IT CAME FOR US. DON'T LET IT REACH THE SHIP."),
  ],
]
# debriefs by outcome: [stage][0 escaped, 1 killed, 2 captured]
OUTRO = [
  [ # 1
    [ (SC_KRAKEN,  'static', 0, "IT TORE THE NET AND DOVE INTO THE ICE. GONE."),
      (SC_SHIP,    'vex',    0, "PARTIAL PAY. BRIGHT BASIN IS NOT IMPRESSED."),
      (SC_SHIP,    'cap',    0, "NEITHER AM I. WE FLY CLEANER NEXT TIME.") ],
    [ (SC_KRAKEN,  'cap',    1, "TARGET DOWN. LANE IS CLEAR."),
      (SC_SHIP,    'vex',    1, "FULL PAY. NOT TRIPLE. BUT FULL."),
      (SC_SHIP,    'forge',  0, "SHE NEEDS NEW PAINT. AGAIN."),
      (SC_SHIP,    'doc',    1, "AND NOBODY GOT BIT. I CALL THAT A GOOD DAY.") ],
    [ (SC_KRAKEN,  'forge',  1, "RUNE NET HOLDING! ONE SPACE KRAKEN, GIFT WRAPPED!"),
      (SC_SHIP,    'doc',    1, "IT'S SULKING IN THE HOLD. IT HAS A CUTE BEAK."),
      (SC_SHIP,    'vex',    1, "TRIPLE PAY! I'M FRAMING THIS INVOICE."),
      (SC_SHIP,    'cap',    1, "THAT'S HOW PROS DO IT. NEXT CONTRACT.") ],
  ],
  [ # 2
    [ (SC_TBIRD,   'static', 0, "IT RODE A LIGHTNING BOLT OUT OF THE STORM."),
      (SC_SHIP,    'vex',    0, "THE GUILD PAID US IN... COUPONS. PARTIAL PAY."),
      (SC_SHIP,    'forge',  0, "MY EYEBROWS ARE STILL SMOKING. FOR COUPONS.") ],
    [ (SC_TBIRD,   'cap',    1, "THUNDERBIRD DOWN. STORM'S BREAKING UP."),
      (SC_STORM,   'static', 1, "BEACONS COMING BACK ONLINE. ALL OF THEM."),
      (SC_SHIP,    'vex',    1, "RELAY GUILD PAID IN FULL. NICE FLYING.") ],
    [ (SC_TBIRD,   'forge',  1, "NETTED! THE WARDS ARE SOAKING UP ITS LIGHTNING!"),
      (SC_SHIP,    'static', 1, "WE HAVE FREE POWER FOR A MONTH. IT'S A BATTERY."),
      (SC_SHIP,    'doc',    1, "IT'S A LIVING CREATURE, STATIC. NOT A BATTERY."),
      (SC_SHIP,    'vex',    1, "TRIPLE PAY AND FREE POWER. BEST. DAY. EVER.") ],
  ],
  [ # 3
    [ (SC_JDEVIL,  'static', 0, "IT SCREAMED AND FLAPPED OFF INTO THE DARK."),
      (SC_BARRENS, 'doc',    0, "THE SETTLERS ARE STILL SLEEPING WITH THE LIGHTS ON."),
      (SC_SHIP,    'cap',    0, "PARTIAL PAY. WE'LL BE BACK FOR IT.") ],
    [ (SC_JDEVIL,  'cap',    1, "DEVIL'S DONE. NO MORE SCREAMING."),
      (SC_BARRENS, 'doc',    1, "THE SETTLERS CAN TURN THEIR LIGHTS OFF."),
      (SC_SHIP,    'vex',    1, "FULL PAY. AND A FREE PIE. THEY LIKE US.") ],
    [ (SC_JDEVIL,  'forge',  1, "COLD IRON NET! ONE JERSEY DEVIL, BAGGED!"),
      (SC_SHIP,    'doc',    1, "IT'S TRYING TO EAT THE NET. IT'S ALSO PURRING?"),
      (SC_SHIP,    'static', 0, "PLEASE DON'T NAME IT, SUTURE."),
      (SC_SHIP,    'doc',    1, "HIS NAME IS GERALD."),
      (SC_SHIP,    'vex',    1, "GERALD IS WORTH TRIPLE. HI, GERALD.") ],
  ],
  [ # 4
    [ (SC_WENDIGO, 'static', 0, "IT CLAWED BACK UNDER THE ICE. IT'S STILL DOWN THERE."),
      (SC_FROST,   'vex',    0, "THE MINERS PAID HALF. THEY'RE NOT GOING BACK DOWN."),
      (SC_SHIP,    'cap',    0, "THEN NEITHER ARE WE. YET.") ],
    [ (SC_WENDIGO, 'cap',    1, "WENDIGO'S FINISHED. MOON IS SAFE."),
      (SC_FROST,   'doc',    1, "THE MINERS CAN GO HOME. NOBODY GOT EATEN."),
      (SC_SHIP,    'vex',    1, "MINERS' GUILD PAID IN FULL. WE'RE RICH! FOR A WEEK."),
      (SC_SHIP,    'static', 0, "CAPTAIN. NEW SIGNAL. FLATWOODS. SOMETHING BIG.") ],
    [ (SC_WENDIGO, 'forge',  1, "FIRE RUNES ARE HOLDING! WENDIGO IN THE HOLD!"),
      (SC_SHIP,    'doc',    0, "I'M KEEPING THE HOLD AT FREEZING. FOR ITS COMFORT."),
      (SC_SHIP,    'vex',    1, "TRIPLE PAY! I'M BUYING US ALL NEW BOOTS."),
      (SC_SHIP,    'static', 0, "CAPTAIN. NEW SIGNAL. FLATWOODS. SOMETHING BIG."),
      (SC_SHIP,    'cap',    1, "THEN THAT'S OUR NEXT BOUNTY.") ],
  ],
  [ # 5
    [ (SC_FLAT,    'static', 0, "IT FOLDED INTO THE DARK. NO TRAIL. NOTHING."),
      (SC_SHIP,    'vex',    0, "THE BOARD PAID A THIRD. I THINK IT'S LAUGHING AT US."),
      (SC_SHIP,    'cap',    0, "IT KNOWS OUR SHIP NOW. WE'LL MEET AGAIN.") ],
    [ (SC_FLAT,    'cap',    1, "MONSTER DOWN. SAUCERS SCATTERING."),
      (SC_SHIP,    'static', 1, "HELM'S BACK. I MISSED YOU, HELM."),
      (SC_SHIP,    'vex',    1, "PAID IN FULL. BY NOBODY. I CHECKED TWICE."),
      (SC_SHIP,    'doc',    0, "SO WHO POSTED THE BOUNTY?") ],
    [ (SC_FLAT,    'forge',  1, "RUNE NET HOLDING! ONE FLATWOODS MONSTER, IN THE HOLD!"),
      (SC_SHIP,    'doc',    1, "IT'S JUST FLOATING THERE. STARING. IT WAVED AT ME."),
      (SC_SHIP,    'vex',    1, "TRIPLE PAY! FROM AN ACCOUNT THAT DOESN'T EXIST!"),
      (SC_SHIP,    'static', 0, "CAPTAIN. IT'S TRANSMITTING. FROM INSIDE THE HOLD."),
      (SC_SHIP,    'cap',    0, "TO WHO?") ],
  ],
  [ # 6 (0 got away, 1 unused, 2 got away with the brig pod: Graul and the gang's ledger)
    [ (SC_CRASH,   'static', 0, "WE'RE DOWN. MOSTLY IN ONE PIECE."),
      (SC_CRASH,   'vex',    0, "NOBODY PAYS US FOR RUNNING. I CHECKED."),
      (SC_CRASH,   'forge',  0, "GUNS BACK IN A DAY. MAYBE TWO."),
      (SC_CRASH,   'cap',    0, "NUBS WON'T LAND HERE. NEITHER SHOULD WE HAVE.") ],
    [ (SC_CRASH,   'static', 0, "WE'RE DOWN. MOSTLY IN ONE PIECE."),
      (SC_CRASH,   'cap',    0, "NUBS WON'T LAND HERE. NEITHER SHOULD WE HAVE.") ],
    [ (SC_CRASH,   'static', 1, "WE'RE DOWN. AND WE BROUGHT A SOUVENIR."),
      (SC_SHIP,    'forge',  0, "THEIR BRIG POD. SOMETHING BIG IS IN IT."),
      (SC_SHIP,    'graul',  0, "GRAUL. BOUNTY HUNTER. NUBS CAGED ME FOR MY MARKS."),
      (SC_SHIP,    'graul',  1, "SO I TOOK HIS LEDGER ON THE WAY OUT."),
      (SC_SHIP,    'vex',    1, "EVERY JOB THE GANG WAS CHASING? THOSE ARE OURS NOW."),
      (SC_CRASH,   'graul',  0, "I OWE YOU A HUNT. THIS ROCK HAS PLENTY TO HUNT."),
      (SC_CRASH,   'cap',    1, "WELCOME ABOARD, WOLF. WATCH THE TREE LINE.") ],
  ],
  [ # 7 (0 the watcher slipped away, 1 the guns killed it, 2 sealed in the hold)
    [ (SC_CRASH,   'graul',  0, "IT RAN. THINGS THAT RUN COME BACK."),
      (SC_CRASH,   'vex',    0, "NO BODY, NO BOUNTY. YOU KNOW THE RULES."),
      (SC_SHIP,    'forge',  0, "GUNS ARE UP. ENGINES BY MORNING."),
      (SC_SHIP,    'cap',    0, "THEN WE LIFT AT DAWN. SLEEP WITH ONE EYE OPEN.") ],
    [ (SC_CRASH,   'forge',  1, "TOLD YOU I'D GET THE GUNS BACK."),
      (SC_CRASH,   'vex',    0, "DEAD PAYS HALF. STILL, DEAD IS QUIET."),
      (SC_SHIP,    'graul',  0, "IT WASN'T ALONE. I HEARD OTHERS CALLING."),
      (SC_SHIP,    'cap',    0, "THEN WE LIFT AT DAWN. BEFORE THEY ANSWER.") ],
    [ (SC_CRASH,   'forge',  1, "RUNE RING HELD. IT'S IN THE HOLD WITH THE REST."),
      (SC_CRASH,   'vex',    1, "A MONSTER THAT HUNTS HUNTERS? I CAN SELL THAT."),
      (SC_SHIP,    'graul',  1, "NOT BAD FOR A CREW WITH NO GUNS."),
      (SC_SHIP,    'doc',    0, "CAPTAIN. EVERYTHING IN THE HOLD JUST WOKE UP."),
      (SC_SHIP,    'cap',    0, "...BECAUSE SOMETHING BIGGER IS CALLING THEM.") ],
  ],
]
# in-flight comms: (id, speaker, text)   15 cols x 3 rows
COMMS = [
    ('CM1_SWARM',   'static', "KRAKENLINGS! A WHOLE BROOD OF THEM!"),
    ('CM1_PODS',    'forge',  "GRAB THE PODS! NEW TOYS INSIDE."),
    ('CM1_ICE',     'static', "ENTERING THE ICE RING. SHOOT ROCKS."),
    ('CM1_NUBS',    'cap',    "IS THAT A GREMLIN IN A GUNSHIP?"),
    ('CM1_OWES',    'vex',    "NUBS! HE OWES ME MONEY. NET HIM!"),
    ('CM1_TANKER',  'doc',    "A HAULER. TORN OPEN LIKE A SNACK."),
    ('CM1_HUGE',    'static', "SOMETHING HUGE. UNDER THE ICE..."),
    ('CM1_KRAKEN',  'cap',    "THERE'S OUR PAYCHECK! SPACE KRAKEN!"),
    ('CM1_HURT',    'doc',    "IT'S BLEEDING INK! KEEP PUSHING!"),
    ('CM1_TENT',    'forge',  "TENTACLES! DON'T GET HUGGED!"),
    ('CM1_GOTAWAY', 'vex',    "NUBS GOT AWAY. HE STILL OWES ME."),
    ('CM1_MARKS',   'doc',    "SUCKER MARKS ON EVERY HULL. CUTE."),
    ('CM1_MIDCAP',  'vex',    "NUBS, NETTED! DEBT PAID IN FULL!"),
    ('CM1_MIDKILL', 'forge',  "SCRAPPED! NUBS BAILED IN A POD."),
    ('CM1_BFLEE',   'static', "IT TORE LOOSE! IT'S GETTING AWAY!"),
    ('CM1_BKILL',   'cap',    "KRAKEN DOWN. CONTRACT CLOSED."),
    ('CM1_BCAP',    'forge',  "RUNE NET HOLDS! KRAKEN IS OURS!"),
    ('CM1_CANCEL',  'forge',  "SHOOT THEIR BULLETS! OUR ROUNDS EAT 'EM."),

    ('CM2_FRONT',   'static', "STORM FRONT AHEAD. HOLD ON TO SOMETHING."),
    ('CM2_MOTHS',   'forge',  "IMPUNDULU! LIGHTNING BIRDS! DIVING!"),
    ('CM2_EELS',    'static', "LINDWORMS IN THE CLOUDS! HIT THE HEAD!"),
    ('CM2_CLOUDS',  'cap',    "THUNDERCLOUDS! MOVE OFF THE MARKERS!"),
    ('CM2_PYLONS',  'doc',    "RELAY PYLONS. SOMETHING IS WATCHING."),
    ('CM2_MOTHMAN', 'vex',    "MOTHMAN! HE'S GOT A BOUNTY TOO!"),
    ('CM2_MMGONE',  'vex',    "MOTHMAN FLEW OFF. THERE GOES A BONUS."),
    ('CM2_EYE',     'static', "WE'RE IN THE EYE OF THE STORM..."),
    ('CM2_TBIRD',   'cap',    "THUNDERBIRD! LET'S CLIP SOME WINGS!"),
    ('CM2_HURT',    'doc',    "IT'S TIRING! DON'T LET UP!"),
    ('CM2_CHARGE',  'forge',  "IT'S CHARGING! WARDS WON'T TAKE THAT!"),
    ('CM2_GUST',    'static', "WIND SHEAR! FIGHT THE STICK!"),
    ('CM2_MIDCAP',  'doc',    "MOTHMAN BAGGED! HE'S... SO FUZZY."),
    ('CM2_MIDKILL', 'static', "MOTHMAN DOWN. LIGHTS OUT."),
    ('CM2_BFLEE',   'static', "IT'S RIDING THE LIGHTNING OUT OF HERE!"),
    ('CM2_BKILL',   'cap',    "THUNDERBIRD DOWN. NICE SHOOTING."),
    ('CM2_BCAP',    'forge',  "NETTED! WARDS ARE EATING ITS SPARK!"),

    ('CM3_WELCOME', 'vex',    "WELCOME TO THE BARRENS. LOVELY."),
    ('CM3_BEHIND',  'static', "PUKWUDGIES! COMING UP FROM BEHIND!"),
    ('CM3_CONES',   'doc',    "DEVIL EGGS! DON'T POP THEM UP CLOSE!"),
    ('CM3_NUBS',    'cap',    "NUBS AGAIN. IN A NEW PILE OF JUNK."),
    ('CM3_OWES',    'vex',    "NUBS! I CAN SEE YOU! PAY UP!"),
    ('CM3_NUBSGONE','vex',    "NUBS ESCAPED. I'M BILLING HIS MOM."),
    ('CM3_FOG',     'doc',    "LIGHTS IN THE FOG. DON'T FOLLOW THEM."),
    ('CM3_WISPS',   'static', "WILL-O'-THE-WISPS! SHOOT WHEN THEY GLOW."),
    ('CM3_DEVIL',   'cap',    "JERSEY DEVIL! DEAD AHEAD!"),
    ('CM3_HURT',    'doc',    "IT'S LIMPING! KEEP AT IT!"),
    ('CM3_FIRE',    'forge',  "FIRE BREATH! FIND THE GAP!"),
    ('CM3_MIDCAP',  'vex',    "GOT YOU, NUBS! NOW PAY UP!"),
    ('CM3_MIDKILL', 'forge',  "JUNK SHIP JUNKED. NUBS EJECTED."),
    ('CM3_BFLEE',   'static', "IT'S FLAPPING AWAY! NO NO NO!"),
    ('CM3_BKILL',   'cap',    "DEVIL'S DONE. BOUNTY CLOSED."),
    ('CM3_BCAP',    'forge',  "COLD IRON NET! THE DEVIL IS OURS!"),

    ('CM4_DESCENT', 'static', "DESCENDING. TEMPERATURE: RUDE."),
    ('CM4_TURRETS', 'forge',  "MINE TURRETS! BOLTED DOWN, STILL MEAN."),
    ('CM4_SNAKES',  'doc',    "TATZELWURMS! CAT FACE. HIT THE HEAD!"),
    ('CM4_MINES',   'vex',    "THE OLD MINES. WATCH THE DOMES."),
    ('CM4_BEAST',   'static', "BIG HEAT SIGNAL. AND IT'S LEAPING!"),
    ('CM4_BEASTNAME','cap',   "BEAST OF BRAY ROAD! BOUNTY ON IT TOO!"),
    ('CM4_BEASTGONE','static',"IT BOLTED ON ALL FOURS. BONUS GONE."),
    ('CM4_WIND',    'static', "ICE STORM! WIND IS SHOVING US!"),
    ('CM4_WENDIGO', 'cap',    "WENDIGO! THERE'S OUR PAYDAY!"),
    ('CM4_HURT',    'doc',    "IT'S WEAKENING! KEEP THE HEAT ON!"),
    ('CM4_CLAWS',   'forge',  "CLAWS! MOVE WHEN IT REACHES!"),
    ('CM4_MIDCAP',  'doc',    "BEAST SEDATED! GOOD... DOGGY?"),
    ('CM4_MIDKILL', 'cap',    "BEAST IS DOWN. STAY SHARP."),
    ('CM4_BFLEE',   'static', "IT'S DIGGING BACK UNDER THE ICE!"),
    ('CM4_BKILL',   'cap',    "WENDIGO'S FINISHED. MOON IS SAFE."),
    ('CM4_BCAP',    'forge',  "FIRE RUNES HOLD! WENDIGO IS OURS!"),

    ('CM5_HELM',    'static', "HELM'S JAMMED! I STEER, YOU SHOOT!"),
    ('CM5_ORBS',    'forge',  "GREEN ORBS! SHOOT THEM DOWN FIRST!"),
    ('CM5_DEBRIS',  'static', "DEBRIS FIELD! BLAST WHAT'S COMING AT US!"),
    ('CM5_ELITE',   'cap',    "RED SAUCER. THAT ONE HITS HARDER."),
    ('CM5_PUSH',    'vex',    "MORE SAUCERS! MORE POINTS! KEEP FIRING!"),
    ('CM5_BOSS',    'cap',    "THERE IT IS. FLATWOODS MONSTER!"),
    ('CM5_BEAM',    'doc',    "ITS EYES ARE CHARGING! HIT IT NOW!"),
    ('CM5_STAGGER', 'forge',  "STAGGERED! IT HATES THAT!"),
    ('CM5_HURT',    'doc',    "IT'S FLICKERING! KEEP SHOOTING!"),
    ('CM5_BFLEE',   'static', "IT'S PHASING OUT! WE LOST IT!"),
    ('CM5_BKILL',   'cap',    "FLATWOODS MONSTER DOWN. GOOD GUNNING."),
    ('CM5_BCAP',    'forge',  "RUNE NET HOLDS! THE MONSTER IS OURS!"),

    ('CM6_EMP',     'static', "EMP HIT! GUNS ARE DEAD! JUST FLY!"),
    ('CM6_GRAZE',   'forge',  "SKIM THEIR SHOTS TO CHARGE BOOST!"),
    ('CM6_MISSILE', 'cap',    "MISSILES! TURN HARD, THEY MISS!"),
    ('CM6_ROCKS',   'doc',    "ASTEROID BELT! AIM FOR THE ARROWS!"),
    ('CM6_LASER',   'forge',  "BLINKING LINES? LASERS! GET CLEAR!"),
    ('CM6_DIVE',    'cap',    "THERE'S THE PLANET! FULL BURN!"),
    ('CM6_HEAT',    'static', "HULL'S GLOWING! SHE'LL HOLD. I HOPE."),
    ('CM6_POD',     'forge',  "THEIR BRIG POD TORE LOOSE! GRAB IT!"),
    ('CM6_GOTPOD',  'vex',    "GOT IT! FINDERS KEEPERS, NUBS!"),
    ('CM6_CLEAR',   'static', "THEY'RE PULLING UP! TOO HEAVY!"),

    ('CM7_START',   'cap',    "STAY ON THE PATH, WOLF. SWING AT ALL."),
    ('CM7_HINT',    'forge',  "YOUR MACE STOPS SHOTS TOO. USE IT!"),
    ('CM7_AMBUSH',  'static', "NUBS'S GROUND CREW! IT'S A TRAP!"),
    ('CM7_CLEAR',   'cap',    "THAT'S ALL OF THEM. KEEP MOVING."),
    ('CM7_BRUISER', 'doc',    "THAT ONE IS BIG. HIT IT A LOT."),
    ('CM7_BRDOWN',  'vex',    "HA! NUBS PAID GOOD MONEY FOR HIM."),
    ('CM7_NEAR',    'static', "ALMOST HOME. THE WOODS WENT QUIET."),
    ('CM7_WRECK',   'forge',  "KEEP THEM OFF THE HULL! I'M WELDING!"),
    ('CM7_HULL',    'forge',  "THEY'RE CHEWING MY WIRING! GET THEM!"),
    ('CM7_GUNS',    'forge',  "GUNS ARE HOT! TURRET IS YOURS!"),
    ('CM7_DARK',    'doc',    "THE TREES... SOMETHING HUGE IS HERE."),
    ('CM7_EYES',    'cap',    "THOSE EYES. IT'S THE WATCHER!"),
    ('CM7_BFLEE',   'static', "IT'S BACKING OFF INTO THE TREES!"),
    ('CM7_BKILL',   'forge',  "TURRET HAS IT! IT'S GOING DOWN!"),
    ('CM7_BCAP',    'forge',  "RUNE RING IS SHUT! IT'S OURS!"),

    ('CM_BUDHINT',  'doc',    "A SLEEPING BABY CRYPTID! FLY INTO IT!"),
    ('CM_BUDGOT',   'vex',    "NEW BUDDY ON BOARD! NO SALARY!"),
]
# per stage: mid-boss (fled, killed, captured) and boss (fled, killed, captured) comms
MID_COMM = [['CM1_GOTAWAY', 'CM1_MIDKILL', 'CM1_MIDCAP'], ['CM2_MMGONE', 'CM2_MIDKILL', 'CM2_MIDCAP'],
            ['CM3_NUBSGONE', 'CM3_MIDKILL', 'CM3_MIDCAP'], ['CM4_BEASTGONE', 'CM4_MIDKILL', 'CM4_MIDCAP'],
            ['CM5_HURT', 'CM5_HURT', 'CM5_HURT'],            # mission 5 has no mid-boss
            ['CM6_CLEAR', 'CM6_CLEAR', 'CM6_CLEAR'],
            ['CM7_BRDOWN', 'CM7_BRDOWN', 'CM7_BRDOWN']]         # nor has mission 6
BOSS_COMM = [[f'CM{s}_BFLEE', f'CM{s}_BKILL', f'CM{s}_BCAP'] for s in range(1, 6)] + [['CM6_CLEAR', 'CM6_CLEAR', 'CM6_GOTPOD'], ['CM7_BFLEE', 'CM7_BKILL', 'CM7_BCAP']]


def wrap(txt, w, rows):
    lines = textwrap.wrap(txt, w)
    assert len(lines) <= rows, (txt, lines)
    return lines


def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace(chr(10), '\\n') + '"'


def emit():
    # cutscene text is only read by scenes.c, so it lives in that bank (7) and keeps ROM0 free
    S = ["// generated by tools/gen_script.py - edit the script there", "#pragma bank 7", "#include <stdint.h>"]
    for name, stages in (("intro", INTRO), ("outro", [o for st in OUTRO for o in st])):
        for s, tab in enumerate(stages):
            for i, (sc, who, ex, txt) in enumerate(tab):
                S.append(f"static const char {name}{s}_{i}[] = {cstr(chr(10).join(wrap(txt, 12, 6)))};")
            S.append(f"static const uint8_t {name}{s}_meta[] = {{" + ",".join(f"{sc},{CREW.index(who) * 2 + ex}" for sc, who, ex, _ in tab) + "};")
            S.append(f"static const char * const {name}{s}_text[] = {{" + ",".join(f"{name}{s}_{i}" for i in range(len(tab))) + "};")
        S.append(f"const uint8_t * const {name}_meta[] = {{" + ",".join(f"{name}{s}_meta" for s in range(len(stages))) + "};")
        S.append(f"const char * const * const {name}_text[] = {{" + ",".join(f"{name}{s}_text" for s in range(len(stages))) + "};")
        S.append(f"const uint8_t {name}_count[] = {{" + ",".join(str(len(t)) for t in stages) + "};")
    open(os.path.join(OUT, "script_scenes.c"), "w").write("\n".join(S) + "\n")
    # crew lines live in their own bank (ROM0 is full); objs.c copies one into RAM when it is shown
    T = ["// generated by tools/gen_script.py - edit the script there", "#pragma bank 255", "#include <gb/gb.h>", "#include <stdint.h>", "BANKREF(script_comm)"]
    for i, (cid, who, txt) in enumerate(COMMS):
        T.append(f"static const char comm_{i}[] = {cstr(chr(10).join(wrap(txt, 15, 3)))};")
    T.append("const char * const comm_text[] = {" + ",".join(f"comm_{i}" for i in range(len(COMMS))) + "};")
    open(os.path.join(OUT, "script_comm.c"), "w").write("\n".join(T) + "\n")
    L = ["// generated by tools/gen_script.py - edit the script there", "#include <stdint.h>"]
    L.append("const uint8_t comm_who[] = {" + ",".join(str(CREW.index(w)) for _, w, _ in COMMS) + "};")
    L.append("const char * const crew_names[] = {" + ",".join(cstr(NAMES[c]) for c in CREW) + "};")
    ids = [c for c, _, _ in COMMS]
    L.append(f"const uint8_t comm_buddy_hint = {ids.index('CM_BUDHINT')};")
    L.append(f"const uint8_t comm_buddy_got = {ids.index('CM_BUDGOT')};")
    L.append("const uint8_t mid_comm[] = {" + ",".join(str(ids.index(c)) for st in MID_COMM for c in st) + "};")
    L.append("const uint8_t boss_out_comm[] = {" + ",".join(str(ids.index(c)) for st in BOSS_COMM for c in st) + "};")
    open(os.path.join(OUT, "script_data.c"), "w").write("\n".join(L) + "\n")
    H = ["// generated by tools/gen_script.py", "#ifndef SCRIPT_IDS_H", "#define SCRIPT_IDS_H"]
    H += [f"#define {c} {i}" for i, c in enumerate(ids)]
    H += ["#endif"]
    open(os.path.join(OUT, "script_ids.h"), "w").write("\n".join(H) + "\n")
    print("script ok:", [len(t) for t in INTRO], [[len(o) for o in t] for t in OUTRO], len(COMMS), "comms")


if __name__ == "__main__":
    emit()
