# Aeternitas1986 C Port - Improvement Roadmap

This file is now the native C/floppy-scale roadmap. It intentionally keeps the broad old Web Edition goals, but does not claim Web Edition features are complete in the C executable unless listed below.

Status policy:

- `[x]` = verified in the C executable or current C source.
- `[~]` = partially present in C; needs hardening, tests, or parity work.
- `[ ]` = still wanted, not verified complete in C.

## Verified / Partial C-Port Status

- Native `aeternitas64.exe` build exists and runs through stdin.
- Main menu, save/load, help, settings, inventory/equipment, forge, status, notes, recap, and mod surfaces exist.
- Shared terminal frame exists for forge and inventory/equipment screens.
- Parser supports chaining with `;`, `then`, `, then`, and `after that`.
- Parser supports `again` / `repeat` for the previous command chain.
- Parser has typo suggestion/autofix for known top-level verbs.
- Parser now strips common first-person/plain-English wrappers such as `I look around` and `check my inventory`.
- Parser records normalization reasons through `parser-normalize` causality events.
- Native parser regression tool exists: `tools/parser_regression_aeternitas64.py`.
- Forge has hidden material blending, keyword inference, role synergy, and whimsical generated outputs.
- Inventory/equipment has fixed-width terminal layout and row/id equip commands.
- Quick smoke testing and dual-exe comparison tooling exist.
- [~] NPC/merchant relationship logic exists in simplified C form, not full Web Edition parity.
- [~] Causality exists, but parser/social/crafting explanations need clearer grouping.
- [~] Screen framework exists, but save/help/settings/mod screens are not all unified yet.
- [~] Crafting outputs currently persist mostly as item names; generated stat profiles need persistence.
- Inform-class noun resolution, pronoun memory, disambiguation, and relative references are still open work.
- Chained commands that enter full-screen submenus need a screen-framework pass.
- [x] Chained commands after fullscreen zero-turn commands — `handle_line` no longer stops the whole chain when a segment opens a fullscreen UI (`help`, `exits`, `status`, etc.); later segments run after the screen closes. Verified: smoke (`help; whereami`, `exits; brief`), parser regression (`fullscreen chain continues`, `exits then inline command`).
- Artificer/Lucid-Blocks-style mid-to-late-game fusion menu is still open work.
- Minigame host API and individual minigames are still open work for the C build.
- Full Web Edition relationship, economy, quest, parser, and minigame parity remains a goal, not a fact.

## Preserved Goal Backlog

## Game Statistics

- **Goal:** restore and improve broad parser coverage from the Web Edition, without copying its fussy behavior
- **Goal:** preserve the large item/content ambition while keeping the native exe floppy-scale
- **Goal:** keep expanding recovered rooms/areas as they are verified in the C build
- **Goal:** reintroduce minigames through a compact shared C screen/minigame framework

> **C-port note**: This document preserves the full Web Edition ambition, but old `[x]` / `[OK] IMPLEMENTED` claims are not trusted for the native C executable. Treat unchecked items as desired goals until verified in `aeternitas64.exe`.

## NPCs & Relationships

### More NPCs

- **General Store Owner** - Shopkeeper NPC with inventory system, potential romance
- **Blacksmith** - Crafting-focused NPC, can upgrade weapons/armor (web-era claim; verify/rebuild for C)
- **Village Innkeeper** - Another romance option, provides lodging and meals
- **Forest Hermit** - Mysterious NPC with knowledge of ancient secrets
- **Traveling Bard** - Storyteller NPC who shares lore and quests
- **Village Guard** - Law enforcement NPC, can arrest player for crimes
- **Farmer** - NPC with crops/animals, trading opportunities
- **Priest/Priestess** - Religious NPC, can bless items, perform ceremonies

### Relationship System Enhancements

- **Relationship Stages** - Stranger → Acquaintance → Friend → Close Friend → Best Friend → Romantic Interest → Lover → Partner (web-era claim; verify/rebuild for C)
- **Memory System** - NPCs remember gifts, compliments, help, special moments (web-era claim; verify/rebuild for C)
- **Emotional States** - NPCs have dynamic emotional states (lonely, happy, flustered, content) (web-era claim; verify/rebuild for C)
- **Late-Game Commands** - Propose, commit, move in, talk about future/past (web-era claim; verify/rebuild for C)
- **Relationship-Gated Content** - Commands unlock based on relationship stage (web-era claim; verify/rebuild for C)
- **Polyamory Support** - Allow multiple romantic relationships simultaneously
- **Jealousy System** - NPCs react to player's relationships with others
- **Relationship Conflicts** - NPCs can have rivalries/conflicts with each other
- **Group Activities** - Spend time with multiple NPCs together
- **Relationship Milestones** - More specific relationship events (first date, moving in, etc.)
- **Breakup System** - Ability to end relationships, with consequences
- **NPC-to-NPC Relationships** - NPCs can form relationships with each other
- **Family System** - NPCs can have families, introduce player to relatives (Added family data structure to NPCs with parents, siblings, children, spouse. Use 'introduce' or 'meet_family' command when close_friend+ to meet family. Relationship improves when introduced.) (web-era claim; verify/rebuild for C)
- **Relationship Preferences** - NPCs have preferences for gifts, activities, conversation topics (Expanded preference system: gift preferences affect relationship gains (loved=4x, liked=2x, disliked=0.5x, hated=0.2x). Topic preferences affect conversation relationship gains. Activity preferences defined for all NPCs.) (web-era claim; verify/rebuild for C)
- **Relationship Decay** - Relationships slowly decrease if not maintained (Added relationship decay system: friend+ relationships decay 0.1 points every 20 turns if no interaction for 50+ turns. Prevents relationships from staying high indefinitely without maintenance.) (web-era claim; verify/rebuild for C)
- **Relationship Events** - Random events based on relationship status (Added random relationship events: 5% chance per turn. Friend-level events (gifts, shared meals) and romantic events (thoughtful gestures, meaningful moments). Events grant relationship bonuses and items.) (web-era claim; verify/rebuild for C)
- **Relationship Quests** - Special quests tied to relationship milestones
- **Relationship Dialogue Trees** - More branching dialogue based on relationship
- **Relationship Status Display v2** - Comparative relationship timeline view (delta since last visit, trust/romance trend, key triggers)
- **Relationship History** - Detailed log of relationship progression (Added comprehensive relationship history tracking that records all interactions with NPCs - gifts, talks, kisses, hugs, etc. - along with friendship/romance changes and relationship stage transitions. View with 'relationship history [npc]' command.) (web-era claim; verify/rebuild for C)

### NPC Intelligence

- **Memory System** - NPCs remember gifts, compliments, help, special moments (web-era claim; verify/rebuild for C)
- **Emotional States** - NPCs have dynamic emotional states based on interactions (web-era claim; verify/rebuild for C)
- **Time-Aware Dialogue** - NPCs have different dialogue based on time of day (web-era claim; verify/rebuild for C)
- **Location-Aware Dialogue** - NPCs adapt dialogue to their current location (web-era claim; verify/rebuild for C)
- **Relationship-Aware Dialogue** - Dialogue changes based on relationship stage (web-era claim; verify/rebuild for C)
- **Smart Dialogue Parsing** - Better pattern matching and context awareness (web-era claim; verify/rebuild for C)
- [~] **AI Routines** - Core NPCs now follow deterministic time-of-day room rotations in the C build (no RNG, derived from the world clock), and movement is visible through `who all` / `where <npc>` with routine-period and activity labels. Verified via parser regression (`npc routine relocation`). Full web-style daily schedules and richer pathing are still open work.
- **Better Memory System** - NPCs remember more specific details about interactions
- **NPC Opinions** - NPCs form opinions about player's actions
- **Gossip System** - NPCs share information about player with others
- **NPC Goals** - NPCs have their own goals and ask player for help
- **Dynamic Schedules** - More complex NPC routines, seasonal changes
- **NPC Relationships with World** - NPCs react to world events, weather, time
- **NPC Preferences** - NPCs have preferences for activities, gifts, conversation topics
- **NPC Personalities** - More distinct personality traits for each NPC
- **NPC Backstories** - NPCs share their backstories as relationships deepen
- **BDI Model (Belief, Desire, Intention)** - NPCs have beliefs about player, desires/goals, and intentions (current plans)
- **Deeper Emotional Engine** - NPCs gain paranoia, suspicion, comfort, jealousy, fear, motivation (beyond current emotional states)
- **Emotion-Influenced Parsing** - NPCs react defensively to aggressive verbs, may refuse actions, dialogue shifts tone
- **Behavior Arcs** - NPC emotional progression over time with gradual mood shifts
- **Query Logic v2 (Cross-Turn Causality)** - Added persistent causal event logging with player-facing causality inspection (`causality [term]`) and natural why/what-changed/who-heard attribution routing (web-era claim; verify/rebuild for C)
- **Motivation-Based Explanations** - NPCs explain their actions ("I moved to the tavern because it's night.")
- **NPC Reactions** - NPCs react to player's actions and choices
- **NPC Requests** - NPCs ask player for favors or help
- **NPC Conflicts** - NPCs can have conflicts with each other
- **NPC Alliances** - NPCs form alliances based on player actions

## Combat & Combat Alternatives

- **Combat System Overhaul** - More strategic combat with skills/abilities
- **Non-Lethal Options** - Persuasion, stealth, bribery alternatives to combat
- **Combat Animations** - Visual feedback for combat actions
- **Weapon Variety** - Different weapon types with unique properties
- **Armor System** - Armor affects defense, appearance changes
- **Status Effects** - Poison, stun, bleed effects in combat
- **Combat Skills** - Unlockable combat abilities and techniques
- **Boss Fights** - Special challenging encounters
- **Combat Difficulty Settings** - Adjustable challenge level

## Items & Crafting

### Crafting System


- **Crafting Stations** - Workbenches, forges, alchemy tables (web-era claim; verify/rebuild for C)
- **Utility Objects System** - Comprehensive utility objects (furnace, workbench, storage, anvil, cauldron, loom, enchanting_table, cooking_station, alchemy_table, grindstone, tanning_rack, spinning_wheel, kiln, forge, sawmill, quern_stone, distillery, smelter, tannery) with ASCII UI and interaction commands (web-era claim; verify/rebuild for C)
- **Material Gathering** - Mining, harvesting, fishing for materials (web-era claim; verify/rebuild for C)
- **Quality System** - Items have quality levels (poor, normal, fine, masterwork) (web-era claim; verify/rebuild for C)
- **Crafting Skill** - Skill-based crafting with failure chance (web-era claim; verify/rebuild for C)
- **Recipe Discovery** - More ways to discover recipes (books, NPCs, experimentation) (Added: Recipe experimentation with skill-based success chance, 3 new recipe books: armor_crafting_manual, advanced_crafting_tome, herbalism_guide) (web-era claim; verify/rebuild for C)
- **Recipe Categories** - Organize recipes by type (weapons, tools, consumables) (web-era claim; verify/rebuild for C)
- **Master Recipes** - Advanced recipes requiring high skill (Added skill requirements to recipes, displayed in craft menu with [OK]/[X] indicators, blocks crafting if skill too low, skill requirements scale with recipe difficulty) (web-era claim; verify/rebuild for C)
- **Enchanting** - Add magical properties to items (Added cmdEnchant with enchanting table requirement, 12 enchantment types across 2 levels: Level 1: fire, ice, lightning, protection, durability, sharpness, speed; Level 2: vampiric, fortification, lifesteal, warding, critical strike. Supports enchantment removal, max 3 enchantments per item, enchantments modify stats appropriately) (web-era claim; verify/rebuild for C)
- **Repair System** - Fix damaged items instead of discarding (Enhanced existing cmdRepair and cmdFix: Blacksmith professional repairs with relationship discounts and varied descriptions; Player repairs with skill-based quality (1-5 durability restored based on crafting skill), repair materials required, crafting skill gain from repairs) (web-era claim; verify/rebuild for C)
- **Item Durability** - Items wear out with use (Added durability loss to cmdUse, cmdMine, cmdHarvest, cmdDig, cmdCut; items break when durability reaches 0; durability display in examine command with condition status (excellent/good/fair/poor/critical); warning messages at low durability; durability already decreases on attack; tools break during gathering activities) (web-era claim; verify/rebuild for C)
- **Crafting Specialization** - Focus on specific crafting types for bonuses (Added cmdSpecialize, specialization bonuses to quality and failure chance, specialization level increases with crafting, max level 20, bonuses: +5% quality per level, -2% failure chance per level) (web-era claim; verify/rebuild for C)
- **Crafting Orders** - NPCs request specific crafted items (Added cmdCraftingOrder, NPCs offer orders based on relationship, accept/decline/complete orders, relationship-based rewards, deadline system, relationship improvements on completion) (web-era claim; verify/rebuild for C)
- **Crafting Contests** - Compete with NPCs in crafting challenges (Added cmdCraftingContest, compete with blacksmith/miller, quality-based judging, rewards for win/tie, relationship improvements, contest progress tracking) (web-era claim; verify/rebuild for C)
- **Recipe Sharing** - Learn recipes from NPCs or find recipe books (web-era claim; verify/rebuild for C)
- **Material Quality** - Higher quality materials = better crafted items (web-era claim; verify/rebuild for C)
- **Crafting Failures** - More varied failure outcomes (partial success, different failure types) (web-era claim; verify/rebuild for C)

### More Items

- **Transformation Items** - Items that modify body attributes (breasts, cock, balls, pussy, etc.) (web-era claim; verify/rebuild for C)
- **Body Attribute System** - Track and modify body attributes dynamically (web-era claim; verify/rebuild for C)
- **NPC Transformation** - Feed transformation items to NPCs (web-era claim; verify/rebuild for C)
- **Cheat Commands** - Manual body attribute editing for testing (web-era claim; verify/rebuild for C)
- **More Transformation Items** - Additional body modification options (from your list)
- **Transformation Reversal** - Items to reverse transformations
- **Gradual Transformations** - Items that change body over time
- **Consumable Buffs** - Temporary stat boosts from food/drinks (web-era claim; verify/rebuild for C)
- **Quest Items** - Unique items required for specific quests
- **Collectibles** - Rare items to find and collect (web-era claim; verify/rebuild for C)
- **Books/Scrolls** - Readable items that teach skills or reveal lore (web-era claim; verify/rebuild for C)
- **Keys & Locks** - Locked doors/chests requiring keys
- **Tools** - Specialized tools for specific tasks
- **Item Sets** - Collecting item sets grants bonuses
- **Legendary Items** - Ultra-rare powerful items
- **Item Upgrading** - Improve existing items with materials
- **Item Enchanting** - Add magical properties to items

## Quests & Story

- **Relationship-Gated Quests** - Quests unlock based on relationship stages (web-era claim; verify/rebuild for C)
- **Quest Objectives** - Track quest progress with objectives (web-era claim; verify/rebuild for C)
- **Quest Rewards** - Reputation, money, items as quest rewards (web-era claim; verify/rebuild for C)
- **Auto-Start Quests** - Quests automatically start when requirements met (web-era claim; verify/rebuild for C)
- **Main Storyline** - Overarching narrative with multiple chapters
- **Side Quests** - More varied side quests with unique rewards
- **Quest Chains** - Multi-part quests that build on each other
- **Dynamic Quests** - Quests that change based on player choices
- **Time-Limited Quests** - Quests that expire after certain time
- **Quest-Integrated Random Events** - Random encounters that can branch, seed, or mutate active quest objectives
- **Secret Quests** - Hidden quests discoverable through exploration
- **Quest Journal v2** - Added dependency-graph view plus failure-state history and branch replay summaries (`journal v2`, `quest graph`, `journal failures`) (web-era claim; verify/rebuild for C)
- **Multiple Endings** - Different endings based on player choices
- **Quest Branches** - Quests with multiple completion paths
- **Quest Failures** - Quests can fail with consequences
- **Daily Quests** - Repeatable daily quests from NPCs (web-era claim; verify/rebuild for C)
- **Quest Sharing** - NPCs can share quests they've heard about
- **Quest Difficulty** - Quests have difficulty ratings
- **Quest Prerequisites** - Quests require completing other quests first

## World & Exploration

### New Areas

- **Dungeon System** - Procedural or hand-crafted dungeons
- **Underground Caverns** - More cave systems to explore
- **Ruins** - Ancient ruins with puzzles and treasures
- **Tower** - Multi-level tower to climb
- **Castle** - Large castle with many rooms
- **Underwater Areas** - Swimmable areas, underwater exploration
- **Sky Areas** - Floating islands or high-altitude locations
- **Other Dimensions** - Portal to alternate realities

### World Features

- **Weather System** - Rain, snow, fog affecting gameplay (web-era claim; verify/rebuild for C)
- **Day/Night Cycle** - More pronounced time effects (web-era claim; verify/rebuild for C)
- **Seasons** - Seasonal changes affecting world and NPCs (web-era claim; verify/rebuild for C)
- **Random Encounters** - Chance encounters while traveling (web-era claim; verify/rebuild for C)
- **Hidden Areas** - Secret locations discoverable through exploration (web-era claim; verify/rebuild for C)
- **Fast Travel** - Quick travel between known locations (web-era claim; verify/rebuild for C)
- **Landmarks** - Notable locations that can be marked (web-era claim; verify/rebuild for C)

## Physics & Interaction

- **Surface Slip Physics** - Movement and physical actions now account for slippery/wet surfaces with fall risk, item drops, and health consequences (web-era claim; verify/rebuild for C)
- **Push Force vs Mass Simulation** - `push` now uses player strength and item mass to determine full movement, partial movement, or strain outcomes (web-era claim; verify/rebuild for C)
- **Pull Resistance & Linkage Feedback** - `pull` now handles resistance by mass and provides mechanism/linkage responses for rope/chain/vine style targets (web-era claim; verify/rebuild for C)
- **Throw Trajectory & Impact Scaling** - `throw` now supports no-target throws, weighted accuracy, and impact damage scaling based on projectile mass (web-era claim; verify/rebuild for C)
- **Kick Recoil & Hardness Response** - `kick` now includes hardness/mass recoil behavior and better physical risk on heavy rigid objects (web-era claim; verify/rebuild for C)

## Character Progression

- **Skill System** - Crafting, trading, engineering, linguistics, survival, alchemy (web-era claim; verify/rebuild for C)
- **Reputation System** - Track reputation with NPCs and factions (web-era claim; verify/rebuild for C)
- **Reputation Levels** - Descriptive reputation levels (revered, hated, etc.) (web-era claim; verify/rebuild for C)
- **Character Creation** - Gender, genitalia, body attributes selection (web-era claim; verify/rebuild for C)
- **Level System** - Character levels with stat increases (Enhanced: Level up grants +5 max health, +2 attribute points, +1 skill tree point. XP bonuses from perks. XP displayed in score. Use 'level' command.) (web-era claim; verify/rebuild for C)
- **Skill Trees** - Unlockable skills in different categories (Enhanced: Skill trees with requirements (level, skill levels, prerequisites). Skills have real effects (-20% crafting costs, +30% quality, etc.). Use 'skilltrees' to view, 'unlock skill [name]' to unlock. Costs skill tree points.) (web-era claim; verify/rebuild for C)
- **Skill Specialization** - Focus on specific skills for bonuses (Enhanced: Specialization levels (1-7) providing +15% to +50% effectiveness. Improves with use. View levels with 'specialize' command.) (web-era claim; verify/rebuild for C)
- **Skill Synergies** - Skills that work together (crafting + engineering) (Enhanced: Synergies with specific effects (quality, price, discovery bonuses). Crafting+engineering (+15% quality), trading+linguistics (+10% prices), survival+alchemy (+12% discovery).) (web-era claim; verify/rebuild for C)
- **Perks/Traits** - Character traits that affect gameplay (Enhanced: Auto-earned perks at milestones (level 5, 10, 50 rooms, 25 crafts, 3 partners). Perks provide real bonuses (+10-20% XP, -50% relationship decay, etc.). Use 'perks' command.) (web-era claim; verify/rebuild for C)
- **Reputation Tiers** - More granular reputation system (Enhanced: 10-tier system with tier-based effects. Positive tiers provide discounts (5-25%) and quest bonuses (10-30%). Negative tiers have penalties. getReputationDiscount() function for pricing.) (web-era claim; verify/rebuild for C)
- **Title System** - Earn titles based on accomplishments (Added title system with titles earned from exploring rooms, crafting items, completing quests, and building relationships. Type "titles" to view all earned titles. Titles display in score command.) (web-era claim; verify/rebuild for C)
- **Prestige System** - Post-game progression options
- **Skill Books** - Items that teach skills when read
- **Skill Training** - NPCs can train player in skills (Added skill training system: NPCs teach skills based on their expertise. Blacksmith trains crafting/engineering, miller trains survival/crafting, merchants train trading, etc. Requires friend+ relationship. Cost scales with relationship (free for partners/lovers). Grants 1-3 skill points with 10-turn cooldown. Use 'train [skill] with [npc]' command.) (web-era claim; verify/rebuild for C)
- **Skill Mastery** - Mastery levels for each skill
- **Skill Decay** - Skills slowly decrease if not used

## Intimate Content

- **Interactive Sex Scenes** - Multi-stage, branching scenes with player input (web-era claim; verify/rebuild for C)
- **Location-Aware Scenes** - Scenes adapt to location (mill, tavern, etc.) (web-era claim; verify/rebuild for C)
- **Genitalia-Aware Scenes** - Scenes adapt to player's chosen anatomy (web-era claim; verify/rebuild for C)
- **Scene Preferences** - NPCs learn and prefer certain intensities/paces (web-era claim; verify/rebuild for C)
- **Scene History** - Track past scenes and preferences (web-era claim; verify/rebuild for C)
- **Physical Interactions** - Hug, kiss, touch, hold, cuddle, caress, grope, strip, pin, dominate, submit (web-era claim; verify/rebuild for C)
- **Relationship-Gated Content** - Intimate content unlocks with relationship progression (web-era claim; verify/rebuild for C)
- **More Scene Variations** - Additional intimate scene types (romantic, rough, playful, experimental, quickie, slow burn, dominant, submissive) (web-era claim; verify/rebuild for C)
- **Position Variety** - More positions and variations (missionary, cowgirl, doggy, standing, against wall, spooning, sitting, sideways) (web-era claim; verify/rebuild for C)
- **Scene Customization** - Player choices affect scene details (lighting: candlelight, firelight, dim, bright; music; props; position selection) (web-era claim; verify/rebuild for C)
- **Aftercare Scenes** - Post-intimate cuddling/conversation (cuddle, talk, stay commands during aftercare stage) (web-era claim; verify/rebuild for C)
- **Public/Private Options** - Choose where intimate scenes occur (public location detection, warnings, explicit confirmation required) (web-era claim; verify/rebuild for C)
- **Consent System** - More explicit consent mechanics (web-era claim; verify/rebuild for C)
- **Kink System** - Track and explore different preferences (web-era claim; verify/rebuild for C)
- **Scene Replay** - Ability to revisit favorite scenes (web-era claim; verify/rebuild for C)
- **Scene Intensity Levels** - More granular intensity options (web-era claim; verify/rebuild for C)
- **Multi-NPC Scenes** - Intimate scenes with multiple NPCs (web-era claim; verify/rebuild for C)
- **Scene Conditions** - Special scenes based on conditions (pregnancy, transformations, etc.) (web-era claim; verify/rebuild for C)
- **Scene Outcomes** - Different outcomes based on player actions (web-era claim; verify/rebuild for C)
- **Scene Memory** - NPCs remember and reference past scenes (web-era claim; verify/rebuild for C)

## UI & Quality of Life

- **Save/Load System** - Multiple save slots (1-10) (web-era claim; verify/rebuild for C)
- **Help Command** - Comprehensive help system with all commands (web-era claim; verify/rebuild for C)
- **18+ Warning Screen** - Age verification on boot (web-era claim; verify/rebuild for C)
- **Better Inventory UI** - Sort, filter, search inventory (web-era claim; verify/rebuild for C)
- **Inventory Categories** - Organize items by type (web-era claim; verify/rebuild for C)
- **Command History** - Better command history navigation (web-era claim; verify/rebuild for C)
- **Settings Menu** - Graphics, sound, gameplay options (web-era claim; verify/rebuild for C) (basic: explicit content, accessibility, modal UI)
- **Save Slots** - Multiple named save slots (web-era claim; verify/rebuild for C)
- **Auto-Save** - Automatic saving at key moments (web-era claim; verify/rebuild for C)
- **Hints System** - Contextual hints when stuck (web-era claim; verify/rebuild for C)
- **Expanded Status Readout** - `status` now shows time-of-day, weather/season, equipped gear, and NPC count in current room (web-era claim; verify/rebuild for C)
- **Checklist Notes** - `notes done <n>` / `notes undone <n>` for lightweight task tracking (web-era claim; verify/rebuild for C)
- **Notes Search** - `notes find <text>` to quickly filter notes by keyword (web-era claim; verify/rebuild for C)
- **Weather Report Command** - Added `weather` / `weather forecast` for current conditions, season, temperature, and short forecast window (web-era claim; verify/rebuild for C)
- **Advanced Wait Command** - Added `wait <hours>` and `wait until <morning|afternoon|evening|night>` with proper multi-turn time advancement (web-era claim; verify/rebuild for C)
- **Targeted Long Rest** - Added `rest until <morning|afternoon|evening|night>` for controlled long rests with cumulative recovery (web-era claim; verify/rebuild for C)
- **Clock-Time Queries** - Added `time until <HH[:MM]>` and `time until <H:MMam/pm>` for exact clock-target checks (web-era claim; verify/rebuild for C)
- **Weather Impact Readout** - Added `weather impact`/`weather effects` to summarize visibility, traversal, and gathering penalties (web-era claim; verify/rebuild for C)
- **Temperature Unit Toggle** - Added `temperature c` / `temperature f` (default shows both C/F) (web-era claim; verify/rebuild for C)
- **Notes Cleanup Command** - Added `notes purge done` to remove completed notes in bulk (web-era claim; verify/rebuild for C)
- **Notes Progress Summary** - Added `notes stats` to show total/todo/done counts and completion percentage (web-era claim; verify/rebuild for C)
- **Accessibility Options** - Text size, color blind mode, etc. (web-era claim; verify/rebuild for C)
- **Relationship Status UI** - Visual display of relationship levels (web-era claim; verify/rebuild for C)
- **Skill Display UI** - Better visualization of skill levels (web-era claim; verify/rebuild for C)
- **Quest Journal UI** - Better quest tracking interface (web-era claim; verify/rebuild for C)
- **Trade History UI** - View past trading transactions (web-era claim; verify/rebuild for C)
- **Crafting UI** - Visual crafting interface with recipe browser (web-era claim; verify/rebuild for C) (ASCII-based)
- **NPC Status UI** - Display NPC location, emotional state, relationship (web-era claim; verify/rebuild for C) (ASCII-based)
- **Utility Objects UI** - ASCII-based display of utility objects in rooms with status, fuel, temperature, capacity, and interaction options (web-era claim; verify/rebuild for C)
- **Who Is Here Command** - Added `who` with direct NPC presence/activity checks (web-era claim; verify/rebuild for C)
- **Progress Command** - Added `progress` for exploration and quest summary (web-era claim; verify/rebuild for C)
- **Nearby Scan Command** - Added `nearby` to inspect adjacent rooms with lock/NPC context (web-era claim; verify/rebuild for C)
- [x] **Global NPC Where Lookup** - `where <npc>` reports live NPC location even when not in the current room, and scheduled NPCs include the current routine period (`afternoon routine`, `night routine`, etc.). Verified via parser regression (`npc routine relocation`).
- [x] **Exit NPC Density Tags** - `exits` now surfaces destination NPC counts (`[npc:n]`). Verified via parser regression (`exit npc density tags`) and smoke.
- [x] **Extended Trail Depth** - `trail <n>` now supports adjustable history depth (up to 25). Verified via parser regression (`trail depth view`, `trail depth bounds`) and smoke.
- [x] **Encumbrance-Aware Hints** - `hint` now calls out rising pack bulk, points players to `loot weight`, and surfaces the heaviest carried anchors for drop/sell triage. Verified via parser regression (`encumbrance aware hints`) and smoke.
- [x] **Exit Lock Indicators** - `exits` displays `[open]`/`[locked]` per route, and filtered lock views now have explicit regression coverage for both locked and unlocked house-area paths. Verified via parser regression (`exit lock indicators locked`, `exit lock indicators open`, `filtered locked exits present`) and smoke.
- [x] **Targeted Look Alias** - `look <target>` and shorthand `l <target>` now route to focused examine behavior. Verified via parser regression (`targeted look alias`, `targeted look short alias`) and smoke.
- [x] **Action Target Cleanup** - Parser strips helper target phrases in natural input (`take a look at ...`, `talk with ...`, `speak to ...`, `look at ...`) down to focused examine/talk commands. Verified via parser regression (`look helper phrase cleanup`, `talk helper phrase cleanup`) and smoke.
- [x] **Plain-English Intent Bridge** - Descriptive phrasing (`I want to...`, `can I...`, `let me...`) resolves to direct commands through the natural-language strip/rewrite pipeline, with explicit regression coverage for movement, inventory, and inspection phrasing. Verified via parser regression (`plain english movement bridge`, `plain english inventory bridge`, `plain english inspect bridge`) and smoke.
- [x] **NPC Non-Collectable Enforcement** - `take` / `grab` / `pick up` now reject live room NPCs with an explicit "people are not inventory" guard instead of falling through to generic missing-item text. Verified via parser regression (`npc non collectable enforcement`) and smoke.
- [x] **Protective Grab Intent Path** - Long rescue phrasing like `grab her hand and pull her away from danger gently` now routes to a dedicated protective action, resolves pronoun targets against the live room NPC, and narrates the move as rescue rather than pickup/possession. Verified via parser regression (`protective grab intent path`) and smoke.
- [x] **Bulk All/Except Parsing** - `take/drop all` and `... except ...` now parse robustly across natural exclusion lists (`and`, `plus`, `&`, articles, comma/semicolon-separated names) instead of treating human phrasing as one raw token. Verified via parser regression (`bulk take except parsing`, `bulk drop except parsing`) and smoke.
- [x] **Bulk Action State Consistency** - Bulk item verbs execute through the same single-item handlers, so `drop all` still clears readied/equipped state and bulk variants keep item-specific side effects instead of bypassing them. Verified via parser regression (`bulk action state consistency`) and smoke.
- [x] **Default Help Output** — `help` shows a contextual smart hint (dark room without light / NPC likely present / general nudge), a compact command-group map, then the full reference (`fill_help_smart_hint`, `fill_help_text` in `aeternitas64_ascii.c`). Verified via smoke autotest.

## Economy & Trading

- [x] **Bartering System** - `haggle` / `barter` now negotiates one-item merchant quotes for the next matching `buy` or `sell`, using `CHA`, patron standing, and merchant friendship without permanently mutating the price tables. Verified via parser regression (`bartering system`) plus smoke coverage through the miller path.
- [x] **Bulk Trading** - Merchants now support `buy all`, `sell all`, and `all except ...` variants while reusing the single-item trade path so rapport, prices, readiness cleanup, and trade-history rows stay consistent. Verified via parser regression (`bulk trading`) and smoke.
- [x] **Trading Skill** - `CHA` now acts as a visible trade knack that stacks with merchant rapport: `wares` and `aptitudes` show the current edge, buy prices improve at higher charisma tiers, and qualifying sell offers pay better. Verified via parser regression (`trading skill prices`) plus updated merchant smoke/regression coverage.
- [x] **Trade History** - Completed merchant buys/sells now append to a save-persisted `trade history` / `trade log` / `transactions` ledger, with regression coverage (`trade history ledger`) plus smoke exercise through quicksave/quickload.
- [x] **Relationship Discounts** - Merchant patron standing now visibly improves prices in `wares`, including list-vs-discounted columns once rapport crosses real thresholds. Verified via parser regression (`merchant reputation discounts`) and smoke.
- [x] **Currency Variety** - The purse now presents lore-aligned gold / silver / bronze / copper denominations while remaining a single saved copper-backed total internally; wallet/status output, trade history, merchant prices, `wares`, sample mod tokens, and hidden cash pickups all render/credit formatted purse values, and coin-like slugs now resolve consistently (including `ancient_coin`, `silver_coin`, `bronze_coin`, and `copper_coin`). Verified via parser regressions (`currency variety`, `currency pickups`) plus smoke.
- [x] **Merchant Reputation** - Merchant-specific patron points build through repeated talk/trade/gift interactions, surface in `rapport`, and feed the shop discount/bonus tables per merchant. Verified via parser regression (`merchant reputation discounts`) and smoke.
- **Merchant Stock Rotation** - Merchants get new items over time (web-era claim; verify/rebuild for C)
- **Merchant Quests** - Merchants request specific items, pay premium (web-era claim; verify/rebuild for C)
- **Investment System** - Invest in businesses, earn passive income
- **Property Ownership** - Buy houses, shops, land
- **Trading Routes** - Set up trade between locations
- **Market Fluctuations** - Prices change based on supply/demand
- **Banking System** - Store money, earn interest
- **Auction System** - Bid on rare items
- **Trade Contracts** - Long-term trading agreements with merchants
- **Merchant Relationship Memory v2** - Persistent merchant trust profiles (credit terms, blacklist thresholds, fraud suspicion, negotiation stance)
- **Price Comparison** - Compare prices across different merchants
- **Trade Skills Specialization** - Focus on specific trade types (weapons, food, etc.)
- **Merchant Favor System** - Do favors for merchants to unlock special items

## Magic & Abilities

- **Magic System** - Spellcasting with mana/energy
- **Spell Schools** - Different types of magic (healing, combat, utility)
- **Rituals** - Complex multi-step magical procedures
- **Curses & Blessings** - Magical effects on player/NPCs
- **Spell-Enchant Fusion Layer** - Bind active spells/ritual signatures into equipment for temporary and conditional enchant states
- **Familiar System** - Magical companions
- **Transformation Magic** - Spell-based body modifications

## Mini-Games & Activities

- **Fishing Mini-Game** - Interactive fishing mechanics (Full Terminal Fisher integration with keyboard controls, full-screen overlay, inventory sync) (web-era claim; verify/rebuild for C)
- **Cooking System** - Prepare meals with recipes (Terminal Diner minigame for tavern shifts, full-screen overlay, earnings sync) (web-era claim; verify/rebuild for C)
- **Gardening/Farming** - Terminal Farm minigame for plowing, planting, watering, harvesting with weather sync (web-era claim; verify/rebuild for C)
- **Gambling** - Card games, dice games at tavern (Terminal gambling minigame with Dice Duel and Blackjack, full-screen overlay, money sync) (web-era claim; verify/rebuild for C)
- **Writing** - Write books, poems, letters (Terminal Writer minigame with full-screen overlay, inventory sync, shop system) (web-era claim; verify/rebuild for C)
- **Reading** - Read books, letters, and written works (Terminal reading interface with scrollable content, full-screen overlay) (web-era claim; verify/rebuild for C)
- **Hunting** - Track and hunt animals (Integrated area-gated hunting minigame: track read -> approach pacing -> shot timing, accessible via `hunt` in valid wilderness areas) (web-era claim; verify/rebuild for C)
- **Hunting Tuning Pass** - Added cooldown, retry-capable track read, wait/listen approach action, approach turn-limit, and perfect-shot quality outcomes (web-era claim; verify/rebuild for C)
- **Piano Performance** - Play piano from a music book in rooms that actually contain a piano; starter sheet unlocked, additional sheets collected in-world (web-era claim; verify/rebuild for C)
- **Lockpicking** - Use a findable `lockpick` near locked doors to launch a timing/skill lockpicking minigame and unlock the door on success (web-era claim; verify/rebuild for C)
- **Lockpicking Stealth And Tool Pass** - Added noise-band risk feedback (`LOW/MEDIUM/HIGH`), NPC hearing/suspicion reactions, guard movement response, canine bark alerts, and tool-specific behavior (rusty/fine pick, tension wrench, skeleton key) (web-era claim; verify/rebuild for C)

## Social Systems

- **Factions** - Join or oppose different groups
- **Faction Reputation** - Reputation with groups, not just individuals
- **Faction Quests** - Quests tied to faction membership
- **Faction Conflicts** - Factions can be at war
- **Social Events** - Festivals, parties, gatherings
- **Gift Giving Events** - Special occasions for gift exchanges
- **Marriage System** - Formal marriage ceremonies
- **Adoption System** - Adopt children or pets

## Pets & Companions

- **Pet System** - Tame and keep animals as pets
- **Pet Care** - Feed, groom, train pets
- **Pet Abilities** - Pets can help in combat/exploration
- **Multiple Pets** - Own multiple pets
- **Pet Breeding** - Breed pets for better stats
- **Companion NPCs** - NPCs that follow player
- **Companion Commands** - Direct companion actions

## Parser Improvements (Historical NLP Systems Integration)

### Advanced Parser Features (From Historical NLP Systems)

- **Query Logic (SHRDLU-style)** - Support questions like "Where is the miller?", "What did I do earlier?", "What's on the table?" (web-era claim; verify/rebuild for C)
- **Reason-About-Objects** - Support relative positioning: "Put the thing you took earlier on the table", "Pick up the book left of the red bottle" (web-era claim; verify/rebuild for C)
- **Slot-Based Meaning Extraction (ATNs)** - Extract manner ("slowly", "gently"), tone ("angrily", "sadly"), purpose ("to help her"), conditions ("if she agrees") (web-era claim; verify/rebuild for C)
- **Tense Recognition** - Understand future ("I will go"), past ("I went"), ongoing ("I am going") (web-era claim; verify/rebuild for C)
- **Dynamic Grammar Templates** - Allow mods to add new grammars ("medieval", "pirate", "demon-curse language") (web-era claim; verify/rebuild for C)
- **Deep Semantic Intent Mapping (Schank's Theory)** - Extract closeness, mood, role intention, target intimacy, social rule violation from actions (web-era claim; verify/rebuild for C)
- **Action Normalization** - Map synonyms to universal primitives: "smack/hit/punch" → ATTACK, "touch/pat/rub" → CONTACT, "kiss/peck" → AFFECTION (web-era claim; verify/rebuild for C)
- **Chain-of-Intent Understanding** - Understand complex intents: "I grab her hand to pull her away from danger" → affectionate + protective (web-era claim; verify/rebuild for C)
- **More Robust Target Resolution (MUD-style)** - Support "attack nearest goblin", "talk to the tall merchant", "give coin to the man in blue" (web-era claim; verify/rebuild for C)
- **Command Chaining with Priority** - Support "take sword; attack goblin", "drink potion then run north" (web-era claim; verify/rebuild for C)
- **Common-Sense Inference (Cyc/Symbolic Logic)** - "If torch is wet, it won't light", "If door is locked, opening fails", "If drop fragile item, it might break" (checkCommonSenseInference function, wet items can't light, locked items need keys, fragile items break on drop) (web-era claim; verify/rebuild for C)
- **Relational World Modeling** - Track inside/under/next to/behind relationships, possession chains, clothing localization (itemRelationships, possessionChains, clothingLocalization tracking systems) (web-era claim; verify/rebuild for C)
- **Effect Propagation** - World reacts logically to changes (if container moves, contents move) (propagateContainerEffects function, containers move with contents when taken/dropped/moved) (web-era claim; verify/rebuild for C)

## Parser Improvements (Zork-Plus Level)

### Current Strengths vs Zork

- [OK] **Systems Depth** - Skills, stealth, crafting, encumbrance, money, reputation, goals, player profile
- [OK] **Player Modeling** - Track behavior and adapt text/hints based on play style
- [OK] **Command Chaining** - Natural multi-step phrasing ("do X. then Y. then Z")
- [OK] **Engine Flexibility** - Clean split between engine/renderer/input and world data/systems

### Where Zork Still Edges Us (To Implement)

- **Preposition Handling** - Support commands like "PUT THE BLUE BALL IN THE LARGE BASKET", "GIVE COIN TO TROLL", "UNLOCK DOOR WITH KEY" (web-era claim; verify/rebuild for C)
- **Preposition Awareness** - Map common prepositions (in/inside/into → IN, on/onto → ON, to/at → TO, with/using → WITH) (web-era claim; verify/rebuild for C)
- **Better Disambiguation** - When multiple items match, ask "Did you mean the rusted key or the ornate key?" and remember the answer (web-era claim; verify/rebuild for C)
- **Edge-Case Grammar Rules** - Handle tons of tiny grammar variations and edge cases (web-era claim; verify/rebuild for C)

## Technical Improvements

- **Performance Optimization** - Improve loading times, reduce lag (web-era claim; verify/rebuild for C)
- **Mobile Support** - Better mobile interface (web-era claim; verify/rebuild for C)
- **Modding Support** - Allow user-created content (web-era claim; verify/rebuild for C)
- **Accessibility** - Screen reader support, keyboard navigation (web-era claim; verify/rebuild for C)
- **Error Handling** - Better error messages and recovery (web-era claim; verify/rebuild for C)
- **Debug Mode** - Developer tools for testing (web-era claim; verify/rebuild for C)
- **Electron Runner Bootstrap Fix** - Smoke/perfect test runners now self-relaunch into true Electron mode when invoked from Node-context wrappers (web-era claim; verify/rebuild for C)
- **NPC Routine Safety Guards** - Added schedule/location validation to prevent post-command runtime failures from invalid NPC routine destinations (web-era claim; verify/rebuild for C)
- **Entity Lookup Helper** - Added centralized `getEntitiesInRoom()` helper used by look/who/exits flows to prevent missing-method runtime errors (web-era claim; verify/rebuild for C)
- **Frustration Help-Level Logic Fix** - Corrected threshold order so `extensive` help can trigger above severe frustration (web-era claim; verify/rebuild for C)
- **Debug Command Parsing Access** - Added synonym entries for one-word debug commands (`debugmode`, `debuginfo`, `debuglog`) (web-era claim; verify/rebuild for C)
- **Locate Alias Support** - Added `locate [thing]` as a direct location-query alias to `where [thing]` (web-era claim; verify/rebuild for C)
- **Where Query Cleanup** - `where` now correctly resolves natural phrasing like `where is guard` (web-era claim; verify/rebuild for C)
- **Nearby Scan Modes** - Added `nearby detail` and `nearby npc` views for faster route/NPC triage (web-era claim; verify/rebuild for C)
- **Lock Recon Command** - Added `lockcheck` to report nearby lock difficulty and available lockpick tools (web-era claim; verify/rebuild for C)
- **Recovery Command** - Added `unstick` to provide immediate context-aware next-step guidance when blocked (web-era claim; verify/rebuild for C)
- **Lockpick Intent Parser Fix** - Fixed `pick lock ...` parsing collision with `pick`/`take` synonym routing (web-era claim; verify/rebuild for C)
- **Lockpick Target Parsing** - Added target extraction for natural lockpick phrases (`pick lock east`, `pick this lock by shed`) (web-era claim; verify/rebuild for C)
- [x] **Filtered Exit Views** - Added filtered exits output (`exits locked/open/new/visited/npc`) and verified via parser regression (`filtered exits view`).
- [x] **Global NPC Presence Scan** - Added world-level NPC view via `who all` / `who global`, listing live NPC placements (static `room.entity` plus deterministic time-of-day routines) with room titles/slugs and a `[here]` marker for the current room. Verified via parser regression (`global who scan`) and smoke.
- [x] **Locked Route Query** - Added `where locks` / `where locked` route lookup support (routes to the lockcheck panel). Verified via parser regression (`locked route query`) and smoke.
- **Lockcheck Toolless Inspection** - `lockcheck` now works even before the player has lockpicking tools (web-era claim; verify/rebuild for C)
- **Contextual Lockpick Fail Output** - Improved lockpick-start failures to include specific nearby lock context (web-era claim; verify/rebuild for C)
- **Progress Diagnostics Expansion** - Added NPC-tracking/region/nearby-lock metrics to `progress` (web-era claim; verify/rebuild for C)
- **Noise/Stealth Telemetry Command** - Added `noise` command with suspicion + guard awareness readout (web-era claim; verify/rebuild for C)
- [x] **Nearby Mode Alias Expansion** - Added `nearby details` / `nearby detailed` parser support. Verified via parser regression (`nearby detailed alias`) and smoke.

### QA & Stability Expansion

- **Seed Matrix Smoke Suite** - Added `test:smoke:matrix` fixed-seed runner with aggregated `smoke_matrix_report.json` output for reproducible regression triage (web-era claim; verify/rebuild for C)
- **Parser Regression Pack** - Added scripted assertions for high-value commands (`look`, `where`, `who`, `nearby`, `exits`, `trail`, `take/drop all`) via `test:regression:parser` with report output (`parser_regression_report.json`) (web-era claim; verify/rebuild for C)
- **Physics Regression Pack** - Added deterministic tests for slip/fall, force-vs-mass push/pull, throw impact scaling, and kick recoil via `test:regression:physics` with report output (`physics_regression_report.json`) (web-era claim; verify/rebuild for C)
- [x] **NPC Schedule Validation Pass** - Boot/load now validate base NPC world references, merchant/dialogue coherence, deterministic routine room slugs, and loaded social-state slugs, then surface counts/warnings in `errors` / `healthcheck`. Verified via parser regression (`npc validation clean on new`, `npc validation clean on load`) and smoke.
- **Error Telemetry Command** - Add player-facing `errors`/`healthcheck` command to summarize recent engine faults for debugging (web-era claim; verify/rebuild for C)
- **Objectives Command** - Added `objectives` for active quest objective snapshots and next-step hints (web-era claim; verify/rebuild for C)
- **Area Scan Command** - Added `scan` for one-screen room intelligence (NPCs, exits, items, objects, resources, lock count) (web-era claim; verify/rebuild for C)
- **Visible Loot Command** - Added `loot [value|weight]` to inspect takeable room items with sort modes (web-era claim; verify/rebuild for C)
- **Route Planner Command** - Added `route <room|npc>` shortest-path guidance with step-by-step directions (web-era claim; verify/rebuild for C)
- **Item Compare Command** - Added `compare <item A> / <item B>` for quick value/encumbrance/durability/category comparison (web-era claim; verify/rebuild for C)
- **Safe Exit Filter** - Added `exits safe` filter (open + visited + no NPC) for lower-risk traversal (web-era claim; verify/rebuild for C)
- **Locked Nearby View** - Added `nearby locked` mode for immediate local lock triage (web-era claim; verify/rebuild for C)
- **Trail Stats Mode** - Added `trail stats` / `trail summary` for path-history analytics (web-era claim; verify/rebuild for C)
- **Trail Reset Control** - Added `trail clear` to reset breadcrumb history when desired (web-era claim; verify/rebuild for C)
- [x] **Where-Am-I Alias** - Added `whereami` as a direct location-introspection shortcut. Verified via parser regression (`whereami alias`).

## Content Expansion

- **More Dialogue** - Expand dialogue trees for all NPCs (Added dialogue patterns for: work, help, thanks, weather, family, food, fire, travel topics to miller, blacksmith, tavern_keeper, bartender, traveling_merchant, general_store_owner) (web-era claim; verify/rebuild for C)
- **Scene Pack Expansion (Per-NPC Targets)** - Add milestone scene sets per core NPC with route conditions and memory callbacks
- **More Items** - Expand item database significantly (Added: compass, whetstone, tinderbox, canteen, bandage, oil_flask, chalk, mirror, candle) (web-era claim; verify/rebuild for C)
- **More Rooms** - Additional explorable locations (Added: watchtower, hidden_shrine) (web-era claim; verify/rebuild for C)
- **More Quests** - Expand quest content
- **More Lore** - Expand world building and backstory
- **More Events** - Random events and encounters (Added 12 new random encounter types across forest, road, village, and wilderness areas) (web-era claim; verify/rebuild for C)
- **More Secrets** - Hidden content to discover (Added: buried_coin, tavern_secret, hidden_scroll, tower_logbook, grove_offering, shrine_offering, ancient_prayer_beads; hidden_shrine area) (web-era claim; verify/rebuild for C)

## Polish & Refinement

- **Better Descriptions** - More detailed room/item descriptions (Enhanced descriptions for lantern, west_of_house, foyer, living_room with more atmospheric detail) (web-era claim; verify/rebuild for C)
- **Narrative Consistency Pass (Linted)** - Enforce style guide, perspective rules, and terminology consistency with scripted content checks
- **Language QA Pass (Automated + Manual)** - Grammar/spelling lint plus curated review for high-frequency rooms/NPC dialogue
- **UI Visual Polish v2** - Advanced readability/contrast pass, spacing system cleanup, and state-feedback hierarchy tuning