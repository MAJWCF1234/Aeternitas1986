# Dual EXE compare (original vs recovered)

- Original: `O:\light\Aeternitas1986\aeternitas64.exe`
- Recovered: `O:\light\Aeternitas1986\aeternitas64_candidate.exe`
- Identical normalized stdout (all jobs): **no**

## global

- return codes: original=0 recovered=0
- identical: **False** (diff hunks ~ 2667 lines in unified diff)

```diff
--- original/global
+++ recovered/global
@@ -390,1040 +390,1627 @@
 ===============================================================================
 
 >> ===============================================================================
-                                   ***   E Q U I P M E N T   &   I N V E N T O R Y   ***                         [ v1.3.
-===============================================================================
- [ EQUIPMENT SLOTS ]                                  [ INVENTORY DATA ]
-+------------------------------------------------+    +----------------------------------------------------------------+
-| [HEAD]    [ Empty ]                            |    | ID    ITEM NAME              TYPE          ATTRIBUTES          |
-| [CHEST]   [ Empty ]                            |    |----------------------------------------------------------------|
-| [HANDS]   [ Empty ]                            |    | [ 1] club                  (accessory)  Def:01 Atk:00 W:01     |
-| [LEGS]    [ Empty ]                            |    | [ 2] bandage               (accessory)  Def:01 Atk:00 W:01     |
-| [FEET]    [ Empty ]                            |    | [ 3] flint                 (accessory)  Def:03 Atk:05 W:02     |
-| [WEAPON]  [ Empty ]                            |    |                                                                |
-| [OFFHAND] [ Empty ]                            |    |                                                                |
-| [ACCESSORY] [ Empty ]                          |    |                                                                |
-+------------------------------------------------+    |                                                                |
-                                                      |                                                                |
-  [ COMBAT STATISTICS ]                               |                                                                |
-+------------------------------------------------+    |                                                                |
-|  Armor Rating : 00      Total Weight : 00      |    |                                                                |
-|  Attack Power : 00      Mobility     : High    |    |                                                                |
-+------------------------------------------------+    +----------------------------------------------------------------+
-===============================================================================
- COMMANDS:[EQUIP #], [EQUIP # TO SLOT], [UNEQUIP SLOT], [DONE]
-
->> ===============================================================================
-  EQUIPMENT
-===============================================================================
-
-Unknown command.
-===============================================================================
-[Press Enter]
-===============================================================================
-                                   ***   E Q U I P M E N T   &   I N V E N T O R Y   ***                         [ v1.3.
-===============================================================================
- [ EQUIPMENT SLOTS ]                                  [ INVENTORY DATA ]
-+------------------------------------------------+    +----------------------------------------------------------------+
-| [HEAD]    [ Empty ]                            |    | ID    ITEM NAME              TYPE          ATTRIBUTES          |
-| [CHEST]   [ Empty ]                            |    |----------------------------------------------------------------|
-| [HANDS]   [ Empty ]                            |    | [ 1] club                  (accessory)  Def:01 Atk:00 W:01     |
-| [LEGS]    [ Empty ]                            |    | [ 2] bandage               (accessory)  Def:01 Atk:00 W:01     |
-| [FEET]    [ Empty ]                            |    | [ 3] flint                 (accessory)  Def:03 Atk:05 W:02     |
-| [WEAPON]  [ Empty ]                            |    |                                                                |
-| [OFFHAND] [ Empty ]                            |    |                                                                |
-| [ACCESSORY] [ Empty ]                          |    |                                                                |
-+------------------------------------------------+    |                                                                |
-                                                      |                                                                |
-  [ COMBAT STATISTICS ]                               |                                                                |
-+------------------------------------------------+    |                                                                |
-|  Armor Rating : 00      Total Weight : 00      |    |                                                                |
-|  Attack Power : 00      Mobility     : High    |    |                                                                |
-+------------------------------------------------+    +----------------------------------------------------------------+
-===============================================================================
- COMMANDS:[EQUIP #], [EQUIP # TO SLOT], [UNEQUIP SLOT], [DONE]
-
->> ===============================================================================
-  EQUIPMENT
-===============================================================================
-
-Unknown command.
-===============================================================================
-[Press Enter]
-===============================================================================
-                                   ***   E Q U I P M E N T   &   I N V E N T O R Y   ***                         [ v1.3.
-===============================================================================
- [ EQUIPMENT SLOTS ]                                  [ INVENTORY DATA ]
-+------------------------------------------------+    +----------------------------------------------------------------+
-| [HEAD]    [ Empty ]                            |    | ID    ITEM NAME              TYPE          ATTRIBUTES          |
-| [CHEST]   [ Empty ]                            |    |----------------------------------------------------------------|
-| [HANDS]   [ Empty ]                            |    | [ 1] club                  (accessory)  Def:01 Atk:00 W:01     |
-| [LEGS]    [ Empty ]                            |    | [ 2] bandage               (accessory)  Def:01 Atk:00 W:01     |
-| [FEET]    [ Empty ]                            |    | [ 3] flint                 (accessory)  Def:03 Atk:05 W:02     |
-| [WEAPON]  [ Empty ]                            |    |                                                                |
-| [OFFHAND] [ Empty ]                            |    |                                                                |
-| [ACCESSORY] [ Empty ]                          |    |                                                                |
-+------------------------------------------------+    |                                                                |
-                                                      |                                                                |
-  [ COMBAT STATISTICS ]                               |                                                                |
-+------------------------------------------------+    |                                                                |
-|  Armor Rating : 00      Total Weight : 00      |    |                                                                |
-|  Attack Power : 00      Mobility     : High    |    |                                                                |
-+------------------------------------------------+    +----------------------------------------------------------------+
-==================================

… (snippet truncated for report; see stdout.diff file)
```

## Interactive / non-stdout gaps

- Arrow-key pagers (`help modding`), splash timing, and some `ui_block_pause` flows only appear in autotest as `[CI autotest: …]` or shortened paths.
- Forge `craft` may bump proficiency via `rand() % 3`, so stdout can diverge even when logic matches.
