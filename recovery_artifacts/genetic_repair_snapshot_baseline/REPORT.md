# Dual EXE compare (original vs recovered)

- Original: `O:\light\Aeternitas1986\aeternitas64.exe`
- Recovered: `O:\light\Aeternitas1986\recovery_artifacts\assistant_temp_recovery_2026-05-07_211828\snapshot_sources\aeternitas64_rebuilt_from_sources.exe`
- Identical normalized stdout (all jobs): **no**

## global

- return codes: original=0 recovered=3221225477
- identical: **False** (diff hunks ~ 1390 lines in unified diff)

```diff
--- original/global
+++ recovered/global
@@ -56,11 +56,25 @@
   N  - north                     |  * mailbox
   S  - south                     |  * scrap_metal
   E  - east                      |  * wood_scrap
-  D  - down                      |  * lockpick
-                                 |  * rusty_pick
+  W  - west                      |  * lockpick
+  U  - up                        |  * rusty_pick
+  D  - down                      |
+  NE - northeast                 |
+  NW - northwest                 |
+  SE - southeast                 |
+  SW - southwest                 |
+  IN - in                        |
+  OUT- out                       |
+  deeper- deeper                 |
+  upstream- upstream             |
+  downstream- downstream         |
+  fountain- fountain             |
+  stage- stage                   |
+  board- board                   |
+  square- square                 |
 ===============================================================================
 [ Turn: 0 | HP: 100/100 | Score: 0 | Light: ON | Rooms: 161 | [help] [menu] [recap] ]
-  Playing as: Autotest Hero â€” Human adventurer
+  [adventurer | Human | COR 0]
 ===============================================================================
 
 >> ===============================================================================
@@ -95,11 +109,25 @@
   N  - north                     |  * mailbox
   S  - south                     |  * scrap_metal
   E  - east                      |  * wood_scrap
-  D  - down                      |  * lockpick
-                                 |  * rusty_pick
+  W  - west                      |  * lockpick
+  U  - up                        |  * rusty_pick
+  D  - down                      |
+  NE - northeast                 |
+  NW - northwest                 |
+  SE - southeast                 |
+  SW - southwest                 |
+  IN - in                        |
+  OUT- out                       |
+  deeper- deeper                 |
+  upstream- upstream             |
+  downstream- downstream         |
+  fountain- fountain             |
+  stage- stage                   |
+  board- board                   |
+  square- square                 |
 ===============================================================================
 [ Turn: 0 | HP: 100/100 | Score: 0 | Light: ON | Rooms: 161 | [help] [menu] [recap] ]
-  Playing as: Autotest Hero â€” Human adventurer
+  [adventurer | Human | COR 0]
 ===============================================================================
 
 >> ===============================================================================
@@ -119,1311 +147,25 @@
   N  - north                     |  * mailbox
   S  - south                     |  * scrap_metal
   E  - east                      |  * wood_scrap
-  D  - down                      |  * lockpick
-                                 |  * rusty_pick
+  W  - west                      |  * lockpick
+  U  - up                        |  * rusty_pick
+  D  - down                      |
+  NE - northeast                 |
+  NW - northwest                 |
+  SE - southeast                 |
+  SW - southwest                 |
+  IN - in                        |
+  OUT- out                       |
+  deeper- deeper                 |
+  upstream- upstream             |
+  downstream- downstream         |
+  fountain- fountain             |
+  stage- stage                   |
+  board- board                   |
+  square- square                 |
 ===============================================================================
 [ Turn: 0 | HP: 100/100 | Score: 0 | Light: ON | Rooms: 161 | [help] [menu] [recap] ]
-  Playing as: Autotest Hero â€” Human adventurer
+  [adventurer | Human | COR 0]
 ===============================================================================
 
->> ===============================================================================
-  EXITS
-===============================================================================
-
-Autotest Hero â€” a Human adventurer Â· they / them / their
-
-West of House â€” sample mod active
-[west_of_house]
-
-Obvious exits: north south east down
-
-Use exits locked/open/new/visited/npc/safe for filters.
-
-  north      North of House               [north_of_house]  [open] [new]
-  south      South of House               [south_of_house]  [open] [new]
-  east       Front Door                   [front_door]  [open] [new]
-  down       Hidden Cellar                [hidden_cellar]  [open] [new]
-
-
---- DLC / mod ---
-[Sample] Hollow Ridge â€” signpost lore or route warnings after Exits.
-===============================================================================
-[Press Enter to return]
-===============================================================================
-  West of House â€” sample mod active
-===============================================================================
-  [ Region: Hollow Ridge ]
-
-  You stand west of a modest house that seems to have misplaced its
-  century. The path is honest dirt; the mailbox is stubborn metal.
-  [000_aeternitas_sample] This blurb comes from your mods folder â€” a
-  tutorial pack the game created on first run. Delete mods/000_aeternitas_sample
-  to restore the original description, or
-  add a higher-priority DLC folder to override this one.
-  â€” Appended by west_of_house.append.txt (layered blurbs for DLC).
---------------------------------------------------------------------------------
-  [ EXITS ]                       |  [ VISIBLE ITEMS ]
-  N  - north                     |  * mailbox
-  S  - south                     |  * scrap_metal
-  E  - east                      |  * wood_scrap
-  D  - down                      |  * lockpick
-                                 |  * rusty_pick
-===============================================================================
-[ Turn: 0 | HP: 100/100 | Score: 0 | Light: ON | Rooms: 161 | [help] [menu] [recap] ]
-  Playing as: Autotest Hero â€” Human adventurer
-===============================================================================
-
->> ===============================================================================
-  EXITS
-===============================================================================
-
-Autotest Hero â€” a Human adventurer Â· they / them / their
-
-West of House â€” sample mod active
-[west_of_house]
-
-Obvious exits: north south east down
-
-Filter: locked
-
-  (No exits match this filter.)
-
-
---- DLC / mod ---
-[Sample] Hollow Ridge â€” signpost lore or route warnings after Exits.
-===============================================================================
-[Press Enter to return]
-===============================================================================
-  West of House â€” sample mod active
-===============================================================================
-  [ Region: Hollow Ridge ]
-
-  You stand west of a modest house that seems to have misplaced its
-  century. The path is honest dirt; the mailbox is stubborn metal.
-  [000_aeternitas_sample] This blurb comes from your mods folder â€” a
-  tutorial pack the game created on first run. Delete mods/000_aeternitas_sample
-  to restore the original description, or
-  add a higher-priority DLC folder to override this one.
-  â€” Appended by west_of_house.append.txt (layered blurbs for DLC).
---------------------------------------------------------------------------------
-  [ EXITS ]                       |  [ VISIBLE ITEMS ]
-  N  - north                     |  * mailbox
-  S  - south                     |  * scrap_metal
-  E  - east                      |  * wood_scrap
-  D  - down                      |  * lockpick
-                                 |  * rusty_pick
-===============================================================================
-[ Turn: 0 | HP: 100/100 | Score: 0 | Light: ON | Rooms: 161 | [help] [menu] [recap] ]
-  Playing as: Autotest Hero â€” Human adventurer
-====================================

… (snippet truncated for report; see stdout.diff file)
```

## Interactive / non-stdout gaps

- Arrow-key pagers (`help modding`), splash timing, and some `ui_block_pause` flows only appear in autotest as `[CI autotest: …]` or shortened paths.
- Forge `craft` may bump proficiency via `rand() % 3`, so stdout can diverge even when logic matches.
