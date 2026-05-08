#include "aeternitas_world_generated.h"
#include <ctype.h>
#include <string.h>

static const char *const EMPTY_LIST[] = {NULL};
static const char *const FOOD_IDS[] = {
  "bread",
  "fresh_bread",
  "cheese",
  "stew",
  "flour",
  "eggs",
  "rations",
  "fish",
  "dried_meat",
  "roasted_meat",
  "meat",
  "fancy_cakes",
  "coffee_beans",
  NULL
};
static const char *const DRINK_IDS[] = {
  "ale",
  "wine",
  "wine_bottle",
  "old_wine",
  "mead",
  "waterskin",
  "stream_water",
  "clear_water",
  "dark_water",
  NULL
};
static const char *const QUEST_HINTS[] = {
  "Listen to The Bartender's Stories — Spend time listening to the bartender's war stories and tales.",
  "Become Friends with The Miller — Build a friendship with The Miller by helping her and getting to know her better.",
  "Help The Miller — Help The Miller with her work at the mill.",
  "Become Partners with The Miller — Reach the highest level of commitment with The Miller.",
  "Win The Miller's Heart — Develop a romantic relationship with The Miller.",
  "Befriend The Tavern Keeper — Build a friendship with the dapper tavern keeper.",
  "Befriend The Traveling Merchant — Build a friendship with the traveling merchant by trading and chatting.",
  "[Daily] Collect Metal for The Blacksmith — The Blacksmith needs scrap metal. Bring 10 scrap_metal.",
  "[Daily] Deliver Ingots to The Blacksmith — The Blacksmith needs iron ingots. Bring 2 iron_ingot.",
  "[Daily] Visit The Blacksmith — The Blacksmith wants to see you. Visit the blacksmith's forge.",
  "[Daily] Deliver Items to General Store — The General Store Owner needs supplies. Bring 5 gold_coin worth of items.",
  "[Daily] Collect Flour for The Miller — The Miller needs some flour collected. Bring her 3 flour.",
  "[Daily] Deliver Supplies to The Miller — The Miller needs some wood scrap for repairs. Bring her 5 wood_scrap.",
  "[Daily] Help The Miller Today — The Miller could use some help. Help her 2 times.",
  "[Daily] Collect Food for The Tavern Keeper — The Tavern Keeper needs ingredients. Bring 3 meat.",
  NULL
};

static const char *const WORLD_SLUGS_REC[WORLD_ROOM_COUNT] = {
  "abandoned_hunters_cabin",
  "abandoned_mill",
  "ancient_forest_gate",
  "ancient_grove",
  "artifact_trader",
  "attic",
  "backyard",
  "backyard_shed",
  "bakery_cart",
  "balcony_view",
  "bard_stage",
  "basement",
  "bathroom",
  "bedroom_hallway",
  "blacksmith",
  "blacksmith_forge",
  "blacksmith_waystone",
  "castle",
  "castle_kitchens",
  "castle_ramparts",
  "cave_deep",
  "cave_depths",
  "cave_entrance",
  "cave_interior",
  "cellar",
  "cellar_tunnel",
  "civic_center",
  "collapsed_mineshaft",
  "community_board",
  "cozy_booths",
  "craftsman_quarter",
  "crystal_catacombs",
  "crystal_pond",
  "cursed_altar_chamber",
  "deep_forest",
  "deep_sanctum",
  "dining_room",
  "early_morning_coffee_shop",
  "east_of_house",
  "exotic_pet_corner",
  "farm",
  "ferry_crossing",
  "festival_stage",
  "fishing_piers",
  "forest_clearing",
  "forest_path",
  "forgotten_armory",
  "fortune_teller_tent",
  "fountain_of_the_sky_rings",
  "fresh_goods_stalls",
  "front_door",
  "garden",
  "general_store",
  "guard_outpost",
  "guest_bedroom",
  "hermit_hut",
  "hidden_back_alcove",
  "hidden_cellar",
  "hidden_cove",
  "hidden_floor_panel",
  "hidden_grove",
  "hidden_shrine",
  "hillside_path",
  "hilltop",
  "hollow_ridge",
  "hollow_ridge_cave",
  "hollowridge_docks",
  "hollowridge_forest",
  "inn_rooms",
  "inside_house",
  "kitchen",
  "library",
  "living_room",
  "lost_cave_dungeon",
  "lower_town_apartments",
  "main_hall",
  "main_living_room",
  "marketplace",
  "master_bathroom",
  "master_bedroom",
  "maze_of_reflection",
  "meadow",
  "mill_upper_level",
  "moonlight_clearing",
  "mountain_cave",
  "mountain_pass",
  "mushroom_circle",
  "nexus_point_1",
  "nexus_point_2",
  "north_of_house",
  "notice_board",
  "old_well",
  "onyx_apartment",
  "pantry",
  "pond",
  "potion_and_alchemy_stand",
  "private_rooms",
  "residential_district",
  "river_bend",
  "river_crossing",
  "river_shore",
  "roasting_room",
  "rocky_outcrop",
  "rooftop_access",
  "rooftop_sipping_area",
  "rose_archway",
  "row_houses",
  "royal_gardens",
  "royal_library",
  "sealed_room",
  "secret_back_room",
  "servant_quarters",
  "shed",
  "smugglers_slip",
  "songbird_grove",
  "south_of_house",
  "stonebridge_homes",
  "stream",
  "stream_source",
  "study",
  "swamp_boardwalk",
  "swamp_clearing",
  "swamp_island",
  "tailor_shop",
  "tavern_back_alley",
  "tavern_back_room",
  "tavern_cellar",
  "tavern_common_room",
  "tavern_exterior",
  "tavern_hallway",
  "tavern_kitchen",
  "tavern_private_room",
  "tavern_privy",
  "tavern_room_1",
  "tavern_room_2",
  "tavern_room_3",
  "tavern_room_4",
  "tavern_stables",
  "tavern_stairs",
  "temple_garden",
  "temple_of_architect",
  "temple_training_grounds",
  "the_hidden_bridge",
  "the_hidden_gem_tavern",
  "throne_room",
  "town_hall",
  "town_square",
  "training_yard",
  "traveling_merchant_row",
  "treasure_room",
  "underground_lake",
  "underground_vaults",
  "village_inn",
  "village_road",
  "village_square",
  "war_council_hall",
  "watchtower",
  "waterfall_of_echoes",
  "west_of_house",
  "whispering_trees_path",
  "woodworkers_guild"
};

static const char *const WORLD_TITLES_REC[WORLD_ROOM_COUNT] = {
  "Abandoned Hunters Cabin",
  "Abandoned Mill",
  "Ancient Forest Gate",
  "Ancient Grove",
  "Artifact Trader",
  "Attic",
  "Backyard",
  "Backyard Shed",
  "Bakery Cart",
  "Balcony View",
  "Bard Stage",
  "Basement",
  "Bathroom",
  "Bedroom Hallway",
  "Blacksmith's Forge",
  "Blacksmith Forge",
  "Waystone - Blacksmith's Forge",
  "Castle Entrance",
  "Castle Kitchens",
  "Castle Ramparts",
  "Deep Cave",
  "Cave Depths",
  "Cave Entrance",
  "Cave Interior",
  "Cellar",
  "Cellar Tunnel",
  "Civic Center",
  "Collapsed Mineshaft",
  "Community Board",
  "Cozy Booths",
  "Craftsman Quarter",
  "Crystal Catacombs",
  "Crystal Pond",
  "Cursed Altar Chamber",
  "Deep Forest",
  "Deep Sanctum",
  "Dining Room",
  "Early Morning Coffee Shop",
  "East of House",
  "Exotic Pet Corner",
  "Village Farm",
  "Ferry Crossing",
  "Festival Stage",
  "Fishing Piers",
  "Forest Clearing",
  "Forest Path",
  "Forgotten Armory",
  "Fortune Teller Tent",
  "Fountain of the Sky Rings",
  "Fresh Goods Stalls",
  "Front Door",
  "Garden",
  "General Store",
  "Guard Outpost",
  "Guest Bedroom",
  "Hermit's Hut",
  "Hidden Back Alcove",
  "Hidden Cellar",
  "Hidden Cove",
  "Hidden Floor Panel",
  "Hidden Grove",
  "Hidden Shrine",
  "Hillside Path",
  "Hilltop Overlook",
  "Hollow Ridge",
  "Hollow Ridge Cave",
  "Hollowridge Docks",
  "Hollowridge Forest",
  "Inn Rooms",
  "Inside House - Foyer",
  "Kitchen",
  "Library",
  "Living Room",
  "Lost Cave Dungeon",
  "Lower Town Apartments",
  "Main Hall",
  "Main Living Room",
  "Marketplace",
  "Master Bathroom",
  "Master Bedroom",
  "Maze of Reflection",
  "Wildflower Meadow",
  "Mill Upper Level",
  "Moonlight Clearing",
  "Mountain Cave",
  "Mountain Pass",
  "Mushroom Circle",
  "Nexus Point - Ancient Grove",
  "Nexus Point - Village Square",
  "North of House",
  "Notice Board",
  "Old Well",
  "Onyx Apartment",
  "Pantry",
  "Tranquil Pond",
  "Potion and Alchemy Stand",
  "Private Rooms",
  "Residential District",
  "River Bend",
  "River Crossing",
  "River Shore",
  "Roasting Room",
  "Rocky Outcrop",
  "Rooftop Access",
  "Rooftop Sipping Area",
  "Rose Archway",
  "Row Houses",
  "Royal Gardens",
  "Royal Library",
  "Sealed Room",
  "Secret Back Room",
  "Servant Quarters",
  "Garden Shed",
  "Smugglers Slip",
  "Songbird Grove",
  "South of House",
  "Stonebridge Homes",
  "Babbling Stream",
  "Stream Source",
  "Study",
  "Swamp Boardwalk",
  "Swamp Clearing",
  "Swamp Island",
  "Tailor Shop",
  "Tavern Back Alley",
  "Tavern Back Room",
  "Tavern Cellar",
  "The Rusty Anchor - Common Room",
  "Outside The Rusty Anchor",
  "Tavern Upper Hallway",
  "Tavern Kitchen",
  "Private Meeting Room",
  "Tavern Privy",
  "Guest Room 1",
  "Guest Room 2",
  "Guest Room 3",
  "Guest Room 4",
  "Tavern Stables",
  "Tavern Stairway",
  "Temple Garden",
  "Temple of The Architect",
  "Temple Training Grounds",
  "The Hidden Bridge",
  "The Hidden Gem Tavern",
  "Throne Room",
  "Town Hall",
  "Town Square",
  "Training Yard",
  "Traveling Merchant Row",
  "Treasure Room",
  "Underground Lake",
  "Underground Vaults",
  "Village Inn",
  "Village Road",
  "Village Square",
  "War Council Hall",
  "Watchtower",
  "Waterfall of Echoes",
  "West of House",
  "Whispering Trees Path",
  "Woodworkers Guild"
};

static const char *const WORLD_BLURBS_REC[WORLD_ROOM_COUNT] = {
  "An old cabin that was once used by hunters. It's been abandoned for some time, but the structure is still sound. Inside, you can find old equipment, furniture, and perhaps some forgotten supplies. The cabin has a mysterious, lonely atmosphere.",
  "An old water mill that has long since fallen into disrepair. The massive wooden water wheel is motionless, caked with mud and algae. The mill building itself has collapsed in places, revealing old machinery inside. Ivy has claimed much of the stone structure, but a ladder still provides access to the upper level. A buff, tough-looking miller works the machinery by the entrance.",
  "An ancient stone gate that stands at the edge of the forest. The gate is covered in moss and carvings that are mostly worn away by time. It's unclear what the gate was meant to guard or mark, but it has an ancient, powerful presence.",
  "A sacred grove where the oldest trees stand in a circle. The air here feels different - charged with ancient energy. A stone altar sits in the center, covered in moss and strange symbols. The grove is quiet, almost reverent, as if holding its breath. You feel like you're standing in a place of great significance. A narrow, almost invisible path leads north into deeper shadows.",
  "A mysterious stall dealing in ancient artifacts, magical items, and rare curiosities. The trader is knowledgeable about the history and properties of each item. Some items glow with residual magic, while others are simply beautiful or historically significant.",
  "The attic is filled with old furniture covered in dust sheets. A small chest sits in the corner, and cobwebs hang from the rafters.",
  "A spacious backyard behind the house. Overgrown grass and wildflowers cover the area, and a stone path leads to a small shed. An old swing hangs from a tree branch, and a vegetable garden plot lies fallow. The area feels peaceful but neglected.",
  "A small wooden shed filled with gardening tools and supplies. Rakes, shovels, and other implements hang from the walls. The air smells of earth and old wood. A small window lets in dim light.",
  "A mobile bakery cart offering fresh bread, pastries, and sweet treats. The aroma of baking bread is irresistible. The baker works from early morning, ensuring everything is fresh and delicious.",
  "A balcony that offers a stunning view of Hollow Ridge. From here, you can see the entire kingdom spread out below - the castle, marketplace, town, and surrounding areas. The view is especially beautiful at sunrise and sunset.",
  "A stage where bards and musicians perform for the tavern's patrons. The stage is well-designed for acoustics, and performances here are always popular. Talented musicians from near and far come to perform on this stage.",
  "The basement is dark and damp. Water drips from somewhere in the darkness, and the air is heavy with the smell of earth and decay.",
  "A shared bathroom with basic amenities. A sink, toilet, and shower stall occupy the small space. The room is functional but shows its age, with worn fixtures and faded tiles.",
  "A narrow hallway leading to the bedrooms. Doors line both sides, and a window at the end looks out over the garden. The hallway is quiet, with only the sound of your footsteps on the wooden floor.",
  "A hot, smoky forge filled with the sound of hammering metal. The air is thick with the smell of coal and heated iron. Anvils, hammers, and tongs are scattered around the workspace. The forge glows with intense heat, and sparks fly as the blacksmith works. Shelves line the walls, displaying finished weapons and armor. The blacksmith, a burly figure with powerful arms and a soot-stained apron, looks up from their work. Outside, a monolithic Waystone structure stands, its Elysium Crystal glowing with magical energy.",
  "A professional blacksmith's workshop in the craftsman quarter. The forge burns hot, and the smith works on various projects. This is a larger, more established operation than the village blacksmith, serving the needs of the entire kingdom.",
  "You stand before a monolithic Waystone structure near the blacksmith's forge. Carved from dark stone and embedded with an Elysium Crystal, the waystone's runes glow with a warm, orange light that reflects the forge's heat. This modern waystone connects to other locations across the realm, allowing instant travel.",
  "The grand entrance to Hollow Ridge Castle. Massive stone walls rise high above, and the royal banner flutters in the breeze. Guards stand watch at the gate, their armor polished to a shine. The castle is an imposing structure, built to last centuries. Inside, you can see corridors leading to various parts of the castle.",
  "A massive kitchen with multiple hearths, ovens, and preparation areas. The air is filled with the aromas of cooking food. Cooks and kitchen staff work busily, preparing meals for the castle's residents. Copper pots and pans hang from the ceiling, and fresh ingredients are arranged on long tables.",
  "The walkway along the top of the castle walls. From here, you have a commanding view of the entire kingdom of Hollow Ridge. Guards patrol the ramparts, keeping watch for any threats. The wind is strong up here, and you can see for miles in every direction.",
  "The deepest part of the cave. The air is still and heavy here. The glowing crystals are more numerous, casting an ethereal light. Ancient markings cover the walls, and you can feel a sense of mystery and power. Something important might be hidden here.",
  "You venture deeper into the cave system. The glow grows stronger, revealing intricate patterns carved into the cave walls. The air hums with barely perceptible energy. In the center of the chamber, a large crystal formation pulses with soft blue light. Something valuable must be hidden here.",
  "The entrance to a small cave in the hillside. The opening is partially hidden by overgrown vegetation. Inside, you can see the cave extends into darkness. The air coming from the cave is cool and damp. Strange sounds echo from within.",
  "The interior of the cave is dark and cool. Stalactites hang from the ceiling, and the floor is uneven with natural stone formations. A small pool of water collects in one corner, and strange crystals grow from the walls, glowing faintly. The cave extends deeper into the hillside.",
  "A dark, stone-walled cellar with wine racks lining the walls. The air is cool and musty.",
  "A tunnel that leads from the tavern's cellar to other parts of the building or even beyond. The tunnel is dimly lit and has an air of mystery. Some say it connects to other locations in the town, though its exact purpose is unclear.",
  "The administrative heart of Hollow Ridge. Important buildings cluster here, including the town hall, community board, and guard outpost. This is where citizens come to conduct official business, report issues, and stay informed about community matters.",
  "An old mineshaft that has partially collapsed. Rubble blocks some passages, but others remain accessible. Old mining equipment lies abandoned, and the area is dangerous but potentially rewarding for those willing to explore carefully.",
  "A public space with notice boards displaying announcements, job postings, community events, and local news. Citizens gather here to stay informed and connect with their neighbors. It's a hub of community information.",
  "Private booths within the coffee shop, perfect for quiet conversation or solitary reflection. The booths are comfortable and well-designed, with soft lighting and privacy.",
  "A district dedicated to skilled craftspeople. Workshops and studios line the streets, each specializing in different trades. The sound of hammers, saws, and other tools fills the air. This is where quality goods are made by hand.",
  "Catacombs where the walls are embedded with glowing crystals. The crystals provide a soft, ethereal light that illuminates the passages. Ancient burial niches line the walls, and the air is cool and still. This is a place of both beauty and solemnity.",
  "A serene pond with crystal-clear water that reflects the sky perfectly. Koi fish swim lazily beneath the surface, and water lilies float on top. A small bridge crosses the pond, and benches offer places to sit and contemplate.",
  "A chamber containing an ancient altar that radiates dark energy. The altar is covered in strange symbols and appears to have been used for dark rituals. The air feels heavy and oppressive, and those who stay too long may feel uneasy.",
  "You venture deeper into the ancient forest. The trees here are massive, their trunks so wide you could hide behind them. Sunlight barely penetrates the thick canopy above, creating a twilight atmosphere. The forest floor is covered in moss and fallen leaves. Strange, glowing mushrooms dot the ground, and you hear the distant call of unknown creatures. In a small clearing, you can see a simple hut - the home of the forest hermit.",
  "The deepest part of the cave system, a sanctum that feels sacred and ancient. The chamber is circular, with symbols carved into the walls that glow with a soft light. This is clearly a place of great significance, though its exact purpose has been lost to time.",
  "A formal dining room with a long wooden table that could seat twelve. Dusty place settings remain on the table, as if waiting for guests who never arrived. A chandelier hangs overhead, its crystals dulled with age. A sideboard holds tarnished silverware and empty serving dishes. Portraits of stern-looking ancestors line the walls.",
  "A cozy coffee shop that opens before dawn, serving the earliest risers. The aroma of freshly roasted coffee fills the air, and the atmosphere is warm and inviting. This is a favorite spot for those who start their day early.",
  "You are on the east side of the house. A well-maintained path leads to a small shed that looks surprisingly intact compared to the house. You can see the front door from here, and notice strange symbols carved into the door frame.",
  "A section of the marketplace dedicated to exotic animals and pets. Cages and enclosures hold various creatures, from colorful birds to small magical beasts. The pet merchant knows how to care for each animal and can help match the right pet to the right owner.",
  "A well-tended farm with fields of crops and pens for livestock. The farmer Jasper works the land with care, growing wheat, corn, potatoes, and carrots. Chickens peck at the ground, and a cow grazes peacefully. The farm provides fresh produce and livestock for the village.",
  "A ferry service that crosses the water to other locations. The ferry is a reliable means of transportation, and the ferryman knows the waters well. Travelers wait here for the next crossing.",
  "A permanent stage built for performances, festivals, and community events. The stage is well-maintained and can accommodate large productions. During festivals, it's the center of entertainment and celebration.",
  "Long piers extending into the water where fishermen cast their lines. The piers are popular with both professional fishermen and hobbyists. The water is teeming with fish, and it's a peaceful place to spend time.",
  "You emerge into a circular clearing surrounded by ancient oaks. In the center stands a moss-covered altar stone, etched with forgotten runes that seem to pulse with faint light. The clearing feels both ancient and alive, as if something powerful once happened here. A weathered journal lies on the altar.",
  "You stand on a narrow, winding path through dense forest. Ancient trees tower overhead, their branches forming a natural canopy. The air is cool and smells of damp earth and pine. Strange sounds echo from deeper in the woods. A rough stone marker stands beside the path, carved with cryptic symbols. To the northeast, you can see a well-traveled road leading toward what appears to be a village.",
  "A hidden armory deep in the caves, filled with old weapons and armor. The equipment is ancient but well-preserved, suggesting it was stored carefully. Some pieces may still be functional, while others are valuable as historical artifacts.",
  "A colorful tent where a fortune teller offers readings and predictions. Mystical symbols decorate the fabric, and the interior is dimly lit with candles. Crystal balls, tarot cards, and other divination tools are arranged on a small table.",
  "A magnificent fountain at the center of the town square. The fountain features rings that seem to float in the air, with water cascading through them in impossible ways. It's a magical marvel that draws visitors and serves as a gathering point.",
  "Stalls overflowing with fresh produce, fruits, vegetables, and locally grown foods. The vibrant colors and fresh scents are inviting. Farmers and vendors display their best wares, calling out prices and specials to passersby.",
  "You are at the front door of the white house. The door is boarded up with heavy planks, but you notice a small keyhole that seems to glow faintly. Strange runes are carved into the door frame, and you feel a chill as you approach.",
  "A peaceful garden with overgrown paths and wildflowers. A small fountain bubbles quietly in the center. Beyond the fountain, an old stone well sits nestled among wild roses.",
  "A well-stocked general store filled with shelves of goods. The air smells of spices, dried goods, and fresh wood. Rows of barrels and crates line the walls, and a polished wooden counter runs along one side. The store owner keeps everything organized and tidy, with items clearly labeled and displayed. A small bell above the door chimes when customers enter.",
  "A small station where the town guards are based. Guards can be seen coming and going, and citizens can report crimes or request assistance here. The outpost is well-equipped and staffed by professional guards dedicated to keeping the town safe.",
  "A smaller bedroom, clearly meant for guests. A simple bed sits against one wall, and a small desk and chair occupy a corner. The room is sparsely furnished but comfortable. A window looks out over the garden.",
  "A simple, weathered hut hidden deep in the forest. The structure is old but well-maintained, blending seamlessly with the natural surroundings. Inside, the hut is filled with ancient tomes, strange artifacts, and the scent of herbs and incense. The hermit's presence fills the space with an aura of ancient wisdom.",
  "A secluded alcove at the back of the coffee shop, hidden from the main area. This is a quiet, private space where sensitive conversations can take place, or where one can simply enjoy solitude.",
  "A secret cellar hidden beneath the house. The air is cool and musty, filled with the scent of old wine and preserved foods. Shelves line the walls, holding jars and bottles covered in dust. A single lantern provides dim light, casting long shadows across the stone floor.",
  "You've discovered a secluded cove, hidden behind a bend in the river. The water is clearer here, reflecting the overhanging trees. An ancient, partially submerged stone structure emerges from the water like a sleeping giant. Strange carvings cover its surface.",
  "A hidden floor panel that can be opened to reveal a secret space below. The panel is well-concealed and requires knowledge or careful searching to discover. What lies beneath is a mystery, but it's clearly meant to be hidden.",
  "A secret grove hidden deep in the forest. Ancient trees form a natural canopy overhead, and rare plants grow in abundance here. The air is thick with magical energy, and you notice strange glowing mushrooms dotting the forest floor. This place feels untouched by time.",
  "A small, secluded shrine hidden deep in the forest, almost completely overgrown with vines and moss. Ancient stone pillars form a circle around a central altar, and strange symbols are carved into the weathered stone. The air here feels different—charged with an ancient energy. Wildflowers grow in profusion around the shrine, and a small stream trickles nearby. This place feels sacred, untouched by time. The shrine seems dedicated to some forgotten deity, their name lost to the ages.",
  "A winding path that leads down the hillside. The path is well-worn, suggesting regular use. Wildflowers grow along the edges, and you can see where small animals have crossed. The path leads back toward the house and forest.",
  "A high hilltop that offers a stunning view of the surrounding countryside. From here, you can see the white house in the distance, the forest stretching out below, and the village beyond. The wind is stronger here, and wild grasses sway in the breeze. A large boulder provides a place to sit and take in the view.",
  "A narrow ridge of weathered stone that cuts through the landscape like a scar. The ridge is hollowed out in places, creating natural caves and overhangs. Ancient carvings mark the stone, their meaning lost to time. The air here feels thin and carries an otherworldly quality. A monolithic Waystone structure stands at the ridge's peak, its Elysium Crystal pulsing with soft light. The ridge offers a commanding view of Hollow Ridge below, with its castle, marketplace, and bustling town. Strange energies seem to gather here.",
  "A natural cave formed within the hollow ridge. The walls are smooth and worn, suggesting this space has been used for centuries. Ancient symbols are carved into the walls, glowing faintly with residual magical energy. A small pool of clear water reflects the cave's ceiling, and strange crystals grow from the walls. The air here is still and sacred, as if this place holds great significance.",
  "A bustling dock area where boats come and go. The docks are well-maintained, with piers extending into the water. Fishermen, traders, and travelers use these facilities. The water is clear, and you can see fish swimming below.",
  "A mystical forest that borders the kingdom. The trees are ancient and tall, and the forest has an otherworldly quality. Strange sounds and lights can sometimes be seen or heard among the trees. This is a place of mystery and natural beauty.",
  "A hallway with several guest rooms. Each room is simple but clean, with a bed, a small table, and a window. The rooms are well-maintained and provide a comfortable place to rest.",
  "You are in the foyer of the mysterious white house. Dust motes dance in shafts of light that filter through the gaps in the boarded windows, creating ethereal beams that illuminate floating particles. The air is thick with the scent of old wood, dust, and something else—something that speaks of forgotten memories and long-lost lives. A grand staircase sweeps upward, its banister carved with intricate patterns now obscured by layers of grime. The steps creak ominously as if warning you of what lies above. Doorways branch off in multiple directions, each leading deeper into the house's mysteries. The floorboards groan underfoot, their ancient wood protesting every step. Portraits line the walls, their subjects' eyes seeming to follow you as you move.",
  "A large kitchen with an old cast-iron stove and a stone sink. Cupboards line the walls, some hanging open to reveal empty shelves. A butcher block table sits in the center, and herbs hang drying from the ceiling. The room smells faintly of old spices and wood smoke. A door leads to a pantry, and another to the backyard.",
  "A small private library with floor-to-ceiling bookshelves. Ladders lean against the shelves, and reading nooks are scattered throughout. The room is quiet except for the sound of pages rustling in the breeze from a cracked window. A small table holds a reading lamp and several open books.",
  "A spacious living room with faded furniture arranged around a cold, dark fireplace. The mantelpiece is carved with intricate designs, now obscured by layers of dust and cobwebs. Tattered curtains hang limply from the windows, their once-vibrant colors faded to muted shades of gray and beige. An old Persian rug covers most of the floor, its patterns worn smooth in places where countless feet have trod. Bookshelves line one wall, filled with dusty volumes whose spines have cracked with age. A comfortable armchair sits near the fireplace, its upholstery torn and stuffing spilling out in places. A coffee table holds an empty vase, its surface marred by water rings and scratches. The air here feels still and heavy, as if time itself has slowed to a crawl.",
  "The entrance to a mysterious cave system that extends deep underground. The entrance is partially hidden and requires some exploration to find. Inside, the caves branch into various chambers and passages, some natural and some clearly worked by ancient hands.",
  "Multi-story apartment buildings that provide affordable housing. The buildings are well-maintained, and the community is diverse. Small shops and services are located on the ground floors, making this a self-contained neighborhood.",
  "The main hall of The Hidden Gem Tavern. The room is spacious and welcoming, with tables arranged for groups of various sizes. A bar runs along one side, and a fireplace provides warmth and ambiance. This is where most of the tavern's activity takes place.",
  "The main living space of the Onyx Apartment. The room is elegantly furnished with quality furniture and tasteful decorations. Large windows let in natural light, and the space feels both luxurious and comfortable.",
  "A vibrant marketplace filled with stalls, carts, and merchants calling out their wares. The air is alive with the sounds of haggling, laughter, and commerce. Colorful awnings provide shade, and the ground is packed earth worn smooth by countless footsteps. This is the commercial heart of Hollow Ridge.",
  "A private bathroom attached to the master bedroom. An old clawfoot bathtub sits in one corner, and a mirror hangs above a sink. The room is clean but shows signs of age, with cracked tiles and tarnished fixtures.",
  "A large bedroom with a four-poster bed draped in faded curtains. A wardrobe stands against one wall, and a vanity table sits beneath a window. The room feels lived-in but abandoned, with personal items still scattered about. A door leads to a private bathroom.",
  "A hedge maze designed for contemplation and meditation. The paths wind in intricate patterns, and mirrors placed at strategic points create interesting visual effects. Those who navigate it often find themselves reflecting on life's journey.",
  "A beautiful meadow filled with wildflowers of every color. Butterflies dance among the blooms, and bees buzz lazily from flower to flower. The grass is tall and soft, and a gentle breeze makes the flowers sway. In the distance, you can see a small pond reflecting the sky. The meadow feels peaceful and alive.",
  "The upper level of the mill is surprisingly intact. Dusty gears and machinery fill the space, their rusted teeth interlocking in ways that suggest complex engineering. A window offers a view of the river below. A sturdy workbench sits in one corner, tools organized with precision.",
  "A clearing in the forest that seems to catch and reflect moonlight in unusual ways. On clear nights, the clearing glows with a soft, silvery light. It's a peaceful, magical place that draws those seeking solitude or inspiration.",
  "The cave opens into a surprisingly large chamber. Stalactites hang like teeth from the ceiling, dripping water into shallow pools below. The walls are adorned with primitive cave paintings depicting strange creatures and symbols. A faint glow emanates from deeper within the cave. The air smells of stone and something else... something ancient.",
  "A narrow mountain pass winds through jagged cliffs. The air is thin and crisp here, and you can see for miles across the wilderness below. Strange rock formations jut from the mountainside, their surfaces marked with ancient symbols. A small cave entrance is visible further up the trail.",
  "A circle of large, colorful mushrooms that grows in a perfect ring. The mushrooms glow faintly with bioluminescent light, and the area feels charged with magical energy. Some believe this is a place where the veil between worlds is thin.",
  "A monolithic structure rises from the center of the grove, its surface covered in intricate runes that pulse with interdimensional energy. This is a Nexus Point - a convergence where the boundaries between dimensions blur. The structure hums with power, and you can sense other locations connected to this network. The runes shift and change, revealing destinations across the realm.",
  "You stand before a monolithic Nexus Point in the village square. The ancient structure, carved from weathered stone and embedded with a large Elysium Crystal, towers above you. Glowing runes cover its surface, shifting and pulsing with interdimensional energy. The crystal at its core radiates a soft, ethereal light. This Nexus Point connects to other locations you've discovered, allowing instant travel between waypoints. To use it, simply trace the destination rune code in the air before the crystal.",
  "You are facing the north side of a white house. There is no door here, just weathered wooden walls that have seen better days. A window is visible but appears sealed shut with thick boards. Ivy creeps up the walls, giving the place an ancient, forgotten feel. A narrow path leads north into the forest.",
  "A large board in the town square where announcements, job postings, and community information are displayed. Citizens regularly check here for news and opportunities. The board is well-organized and kept up to date.",
  "You stand beside an old stone well, its wooden roof weathered and splintered. The well's opening is dark and you can barely make out your reflection in the water below. A rusty metal bucket sits on the stone rim, attached to a frayed rope. Something glints in the depths.",
  "A luxurious apartment building known for its high-quality accommodations. The building is well-maintained and offers premium living spaces. The Onyx Apartment is a prestigious address in Hollow Ridge.",
  "A small pantry filled with empty shelves and storage containers. Jars line the walls, most empty but a few still containing preserved foods. The air is cool and smells of earth and old preserves. A small window lets in dim light.",
  "A small, clear pond surrounded by reeds and water lilies. The water is still and reflects the sky perfectly. Dragonflies dart across the surface, and you can see small fish swimming in the depths. A wooden dock extends into the water, though it looks weathered. The area is peaceful and serene.",
  "A specialized stall filled with potions, elixirs, and alchemical ingredients. Glass bottles of various colors line the shelves, and the air carries the scent of herbs and magical reagents. An alchemist tends the stand, ready to sell or create custom potions.",
  "Private rooms available for rent in the tavern. These rooms offer privacy and comfort for travelers who need a place to stay. The rooms are clean, well-furnished, and provide a good night's rest.",
  "A neighborhood of homes where the citizens of Hollow Ridge live. The streets are clean and well-maintained, with gardens and small yards. Children play in the streets, and neighbors chat on their doorsteps. This is a peaceful, community-oriented area.",
  "The river curves here, revealing a wider expanse of water and sandy banks. Ancient stone pillars rise from the water at regular intervals, suggesting some long-forgotten structure. A rope bridge spans one section, though many of its planks are missing. The far shore seems distant but reachable.",
  "You've reached a strategic point where the river is narrowest. A makeshift raft of logs is tied to a tree stump on the far bank. On this side, a cave opening is partially hidden by hanging vines. The sound of water rushing over rocks echoes from inside.",
  "The slow-moving river spreads out before you. A small wooden dock extends into the water, its planks warped and missing in several places. On the shore, an abandoned rowboat lies overturned, half-buried in mud. Strange symbols are carved into nearby trees, and you can see where the river bends south toward distant hills.",
  "The back room where coffee beans are roasted. The process fills the air with rich, aromatic scents. The roaster takes pride in their craft, carefully controlling temperature and timing to create the perfect roast.",
  "A rocky area where large boulders have tumbled down from the hills above. The rocks are covered in lichen and moss, and small plants grow in the crevices. The area feels ancient and weathered. You can see where animals have made paths between the rocks.",
  "Access to the rooftop of the Onyx Apartment building. The rooftop offers panoramic views and is a peaceful place to relax. Some residents use it for gardening, stargazing, or simply enjoying the fresh air.",
  "An open-air rooftop area where customers can enjoy their coffee while taking in the view of Hollow Ridge. The morning air is fresh, and the sunrise views are spectacular. This is a peaceful, elevated space.",
  "A beautiful archway covered in climbing roses. The flowers bloom in various colors, creating a stunning natural tunnel. The scent is intoxicating, and the archway leads to a special garden area beyond.",
  "A line of connected houses built in a uniform style. Each home has its own character, with different decorations and gardens. The residents take pride in their homes, and the area feels welcoming and safe.",
  "Magnificent gardens maintained for the royal family and open to the public. The gardens are beautifully landscaped with paths winding through various themed areas. Flowers bloom in abundance, and the air is filled with their fragrance.",
  "A vast library filled with towering bookshelves that reach toward the ceiling. Ladders on rails allow access to the highest shelves. Ancient tomes, scrolls, and manuscripts fill every available space. The air smells of old paper and leather bindings. A reading area with comfortable chairs sits near a large fireplace.",
  "A room that has been sealed off, its door locked and perhaps even bricked up. The reason for the sealing is unclear, but it adds an air of mystery to the apartment. Some say the room is haunted, while others believe it contains something valuable or dangerous.",
  "A hidden room at the back of the tavern, accessible only to those who know where to look. This room is used for private meetings, sensitive discussions, or simply as a quiet retreat from the main hall's noise.",
  "The living quarters for the castle's staff. Simple but comfortable rooms line the corridors. The area is well-maintained and organized, reflecting the care the servants take in their work. Personal belongings and small comforts make these spaces feel like home.",
  "A small wooden shed filled with gardening tools and supplies. Sunlight streams through a dusty window.",
  "A hidden, less-traveled area of the docks. Rumors suggest this is where less legitimate activities take place. The area is shadowy and quiet, with hidden nooks and crannies. Those who know where to look can find interesting opportunities here.",
  "A grove of trees where songbirds make their home. The area is filled with the beautiful sounds of birdsong throughout the day. Benches are placed among the trees, allowing visitors to sit and enjoy the natural music.",
  "You are behind the house. A small garden plot lies fallow, choked with weeds and dead plants. A rusty gate leads to what might be a cellar entrance. The air here is thick and musty, and you hear the faint sound of dripping water.",
  "Homes built near a stone bridge that crosses a small stream. These houses are slightly larger and more well-appointed, with stone foundations and quality construction. The bridge itself is a local landmark.",
  "A small stream that flows through the forest, its clear water bubbling over smooth stones. The sound of running water is soothing. Small fish dart in the shallows, and you can see where animals have come to drink. The stream flows from the north and disappears into the forest to the south.",
  "The source of the stream - a natural spring bubbling up from the ground. The water is crystal clear and cold. Ferns and moss grow around the spring, creating a lush, green area. The water flows down a small waterfall into the stream below. This feels like a place of natural power.",
  "A cozy study filled with books and papers. A large desk sits beneath a window, covered in scattered documents and an old inkwell. Bookshelves reach to the ceiling, filled with leather-bound volumes. A globe sits in one corner, and a comfortable reading chair faces the desk. The air smells of old paper and ink.",
  "You carefully navigate the rotting boardwalk that winds through the swamp. The wood creaks ominously under your weight. Strange lights dance in the mist ahead—will-o'-wisps, perhaps? In the distance, an island rises from the murky water with a gnarled old tree at its center. Something glints among its roots.",
  "The forest gives way to a murky swamp. Mist hangs low over stagnant water, and twisted trees protrude from the bog like skeletal hands. Strange sounds echo from the depths—croaking frogs, splashing, and something else you can't quite identify. A rickety wooden boardwalk extends into the swamp, though many planks are missing.",
  "You reach a small island in the center of the swamp. An ancient, gnarled tree dominates the space, its bark twisted and weathered. Thick roots expose something metallic buried beneath. The island feels significant, perhaps sacred. Strange energy emanates from the tree itself.",
  "A fine tailoring establishment where skilled seamstresses and tailors create custom clothing. Fabrics of all kinds line the walls, and finished garments are displayed. The shop offers both everyday wear and special occasion attire.",
  "A narrow alley behind the tavern, used for deliveries and waste disposal. Barrels and crates are stacked against the walls. A door leads into the kitchen, and you can see the stable entrance further down. The alley is dimly lit and somewhat secluded.",
  "A smaller, more private room separated from the main common area. This space is quieter, with a few tables and comfortable chairs. The walls are decorated with old maps and nautical artifacts. A door in the corner leads to what might be a private meeting space. This room is often used for more intimate conversations or business dealings.",
  "A cool, dark cellar beneath the tavern. Barrels of ale and wine line the walls, and shelves hold preserved foods, pickled vegetables, and dried meats. The air is musty with the scent of aging alcohol and earth. A ladder leads back up to the kitchen.",
  "The main room of the tavern is warm and welcoming, filled with the sounds of conversation, laughter, and clinking mugs. A long wooden bar dominates one wall, behind which shelves are stocked with bottles and casks. Several round tables are scattered throughout, some occupied by patrons. A large fireplace crackles merrily, casting dancing shadows. Stairs lead up to the rooms above, and you can see doorways leading to the kitchen and a back room.",
  "You stand before a two-story wooden building with a weathered sign that reads 'The Rusty Anchor Tavern'. The structure looks well-maintained despite its age, with warm light spilling from the windows. A hitching post stands to one side, and a small stable is visible around back. The smell of ale, roasted meat, and wood smoke fills the air. The front door is open, inviting you inside.",
  "A long hallway on the second floor with several doors leading to guest rooms. A window at the end of the hall looks out over the village. The floorboards are well-worn from years of use. Each door has a small number plate.",
  "A bustling kitchen filled with the aromas of cooking. A large hearth with a spit dominates one wall, currently roasting what smells like pork. Pots and pans hang from hooks, and a large wooden table serves as a prep area. Barrels of flour, salt, and other supplies line the walls. A door leads to what appears to be a storage cellar.",
  "A secluded room with thick walls and a heavy door. This space is designed for privacy, with a single table and chairs. The room is dimly lit by a single lantern. This is where sensitive conversations happen, away from prying ears.",
  "A small outbuilding behind the tavern serving as the privy. It's a simple structure with basic facilities. The door can be latched from the inside for privacy.",
  "A simple but comfortable guest room. A bed with a straw mattress sits against one wall, and a small table and chair occupy the corner. A window looks out over the village. The room is clean and well-maintained, if spartan.",
  "Another guest room, similar to the others but with a slightly larger bed. A washbasin sits on a stand, and a small chest at the foot of the bed could hold belongings. The room has a cozy, lived-in feel.",
  "A guest room with a view of the stable yard. This room is slightly larger and includes a small writing desk. The bed looks more comfortable than the others, with a thicker mattress.",
  "The last guest room in the hallway. This one has a small balcony overlooking the back alley. The room is well-furnished with a bed, a wardrobe, and a small table. It's the most private of the guest rooms.",
  "A small stable behind the tavern where travelers can board their horses. The structure is simple but sturdy, with several stalls. Hay is stacked in one corner, and a water trough sits in the center. The smell of hay, horses, and leather fills the air.",
  "A narrow wooden staircase leads up to the second floor. The steps creak slightly underfoot, and the walls are lined with old portraits and tapestries. You can hear muffled sounds from the rooms above.",
  "A peaceful garden surrounding the temple. Beautiful flowers bloom in carefully tended beds, and a small fountain provides the gentle sound of flowing water. The garden is a place of meditation and reflection, where the priestess and missionaries often come to find peace and connect with The Architect's love.",
  "A beautiful, serene temple dedicated to The Architect. White marble columns support a vaulted ceiling, and symbols of The Architect adorn the walls. The air is filled with the scent of incense and the sound of soft prayers. The High Priestess Seraphina presides here, along with her paladin Marcus and missionaries Kira, Yuki, and Elena. The temple radiates peace and acceptance, welcoming all who seek The Architect's love.",
  "A training area where the paladin Marcus practices and trains. Weapons racks line the walls, and training dummies stand ready. The area is well-maintained, reflecting the paladin's dedication to his duty of protection.",
  "A small, hidden bridge that crosses a stream within the gardens. The bridge is partially obscured by overhanging plants, making it easy to miss. It leads to a secluded area of the gardens that few visitors discover.",
  "A popular tavern known for its welcoming atmosphere and quality drinks. The tavern is well-frequented by locals and travelers alike. The main hall is lively, with music, conversation, and good food. It's a place where stories are shared and friendships are made.",
  "A magnificent hall with vaulted ceilings and ornate tapestries. The royal throne sits at the far end on a raised dais, carved from ancient wood and inlaid with precious metals. Stained glass windows cast colorful light across the polished marble floor. This is where the ruler of Hollow Ridge holds court and makes important decisions.",
  "A stately building where the town's administration is conducted. Official meetings, public forums, and important announcements happen here. The building is well-maintained and serves as a symbol of the community's organization and governance.",
  "The heart of Hollow Ridge, a bustling town square where paths converge. A magnificent fountain stands at the center, its waters sparkling in the light. A festival stage dominates one side, and a notice board displays various announcements. The square is surrounded by the various districts of the town, each with its own character and purpose. The kingdom's castle looms to the east, while the marketplace bustles to the south.",
  "An open courtyard where guards and soldiers train. Practice dummies, weapon racks, and training equipment are arranged around the space. The sound of clashing steel and shouted commands fills the air. This is where the kingdom's defenders hone their skills.",
  "A row of stalls set up by traveling merchants from distant lands. Exotic goods, rare items, and unusual wares fill the displays. These merchants bring news from far away and offer items not found elsewhere in the kingdom.",
  "You have discovered a hidden treasure room! Golden light reflects off ancient artifacts and precious gems. This is the ultimate goal of your adventure!",
  "A vast underground lake with still, dark water. The lake is deep and mysterious, and strange things are said to live in its depths. The water reflects the light from above, creating an otherworldly atmosphere. A small boat is tied to a makeshift dock.",
  "Deep beneath the castle, these vaults hold the kingdom's treasures, artifacts, and important documents. Heavy iron doors protect the most valuable items. Torches flicker on the walls, casting dancing shadows. The air is cool and dry, perfect for preservation.",
  "A cozy, welcoming inn with a warm atmosphere. The common room is filled with comfortable chairs and tables, and a crackling fireplace provides warmth and light. The innkeeper, Lydia, greets guests with a warm smile. Rooms are available upstairs, and the inn serves hearty meals. The smell of fresh bread and cooking fills the air.",
  "A well-maintained dirt road winds through the forest toward a small village. The path is wide enough for carts, and you can see wagon ruts in the mud. Smoke rises from chimneys in the distance, and the sound of voices and activity drifts on the breeze. A wooden signpost points toward 'The Rusty Anchor Tavern'.",
  "The central square of a small village. A well sits in the center, and several shops and buildings surround the open space. People go about their daily business, and the atmosphere is lively but peaceful. The Rusty Anchor Tavern is visible to the west. In the center of the square stands a monolithic structure - a Nexus Point that connects to other locations across the realm. A well-maintained path leads north toward the kingdom of Hollow Ridge. The village guard patrols the area, keeping watch.",
  "A strategic planning room with a large table covered in maps and tactical diagrams. Battle standards hang from the walls, and weapons are displayed in cases. This is where military strategies are discussed and important decisions about the kingdom's defense are made.",
  "The highest point of the castle, a tall watchtower that provides an unparalleled view of the surrounding lands. The tower is circular, with narrow windows on all sides. A spyglass sits on a small table, and a signal fire pit is ready to be lit. From here, you can see the entire kingdom spread out below like a map. The wind howls through the windows, and you feel as if you're standing at the edge of the world.",
  "A beautiful waterfall where the sound echoes in unusual ways. The water cascades down a rocky face into a pool below. The echoes create a musical quality, and some say you can hear voices or messages in the sounds if you listen carefully.",
  "You are standing in an open field west of a white house, with a boarded front door. The grass is overgrown and wildflowers dot the landscape, swaying gently in the breeze. A small mailbox stands sentinel near the path, its red flag rusted in the down position. The house looms before you, its windows dark and foreboding like empty eye sockets. The paint has peeled away in patches, revealing weathered wood beneath. A faint breeze rustles the nearby trees, carrying with it the scent of damp earth and something else—something ancient and forgotten. The air feels heavy, as if the very atmosphere is holding its breath.",
  "A path through the forest where the trees seem to whisper secrets. The wind through the leaves creates sounds that almost sound like voices. Some say the trees themselves are trying to communicate with those who pass by.",
  "A guild hall and workshop for skilled woodworkers. Furniture, tools, and decorative items are crafted here. The scent of sawdust and wood polish fills the air. Master craftspeople train apprentices in the art of woodworking."
};

static const char *const WORLD_REGIONS_REC[WORLD_ROOM_COUNT] = {
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge",
  "Hollow Ridge"
};

static const char *const WORLD_ENTITIES_REC[WORLD_ROOM_COUNT] = {
  "",
  "miller",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "blacksmith",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "forest_hermit",
  "",
  "",
  "",
  "",
  "",
  "farmer",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "general_store_owner",
  "",
  "",
  "forest_hermit",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "tavern_keeper",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "priestess",
  "paladin_marcus",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "village_innkeeper",
  "",
  "village_guard",
  "",
  "",
  "",
  "",
  "",
  ""
};

static const unsigned char WORLD_DARK_REC[WORLD_ROOM_COUNT] = {
  0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
  1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int WORLD_EXITS_REC[WORLD_ROOM_COUNT][DIR_COUNT] = {
  {-1, -1, -1, -1, -65469, -1, -1, -1, -1, -1, 6619135, 5374064, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -65469, -1, -1, -1, -1, -1, 4063231, -65502, -1, -1, -1, 5767167, -1, -1, -1, -1},
  {-1, -1, -1, 5111807, -1, -1, -1, -1, -1, -1, -1, -1, -65467, -1, -1, -1, -1, -1, -1},
  {-65466, 458803, -1, -1, -1, -1, -1, -1, -1, -1, 458751, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, 5111807, -1, -1, -1, -1, -1, -65444, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 9437183, -1, -1, -1, -1, -1, -1, -1, -1, 4587519, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65523, -1, -1, -1, -1, -1, -1, -1, 5242879, 4718646, -65524, -1, -1, -1, -1, -1, -1, -1},
  {-1, 10158079, -1, -1, -1, -65520, -1, -1, -1, -1, -65506, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, 983039, -1, -1, -1, 7143423, 10158098, 7274642, 9633943, -65517, 9502719, -1, -1, -1, -1},
  {-65519, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10289151, -1, 1179647, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, 1572863, -1, -1, -1, -1, 9830399, -65452, -1, -1, -1, -1, -1, -1, -1},
  {-65434, -1, -1, -1, -1, -65513, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1310742, -1, -1, -1},
  {-1, -1, -65421, -1, -1, -1, -1, -1, -1, -1, 9437183, -1, -1, -1, -1, -1, -1, -1, -1},
  {3473553, 9568284, -1, -1, -1, -1, -1, -1, -1, 4849663, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, 1769471, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65499, -1, -1, -1, -1, -1, -1, -1},
  {10485775, 9568379, -1, -1, -1, -1, -1, -1, -1, -1, -65463, -1, -1, -1, -1, -1, -1, -1, -1},
  {7077887, -1, -1, -1, -1, -1, -1, -1, -1, -1, 4849663, -1, -1, -1, -1, -1, -1, -1, -1},
  {-65533, -65492, -1, -1, -65491, -65481, -1, -1, -1, -1, -1, -1, 4849663, -1, -1, -1, -1, -1, -1},
  {-65467, -65466, -1, -1, -1, -1, -1, -1, -1, -1, 1900600, 6815845, 9633791, -1, -1, -1, -1, -1, -1},
  {7536729, 10354800, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5111807, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -65382, -1, -1, -1, -1, -1, -1, -65470, -1, -1, -1, -1, -1, -1, -1},
  {9633791, -1, -1, -1, -1, -1, -1, -1, -1, -65390, -65470, -1, -1, -1, -1, -1, -1, -1, -1},
  {2949205, -65476, -1, -1, -1, -1, -1, -1, -1, 2949119, 7929945, -65455, 10092543, -65502, -1, -1, -1, -1, -1},
  {-1, -1, -65463, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5111807, -1, -1, -1, -1, -1},
  {-65390, -1, -1, -1, -1, -1, -1, -1, -1, -65390, -1, -65459, -1, -1, -1, -1, -1, -1, -1},
  {-1, 10354757, -1, -1, -1, -1, -1, -1, -1, 7602175, 6029311, -1, -1, -1, -1, -1, -1, -1, -1},
  {10158079, -1, -1, 10158079, -1, -1, -1, -1, -1, 1769471, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-65523, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65502, -1, -1, -1},
  {-65499, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10420223, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, 6619135, -1, -1, -1, -1, -1, -65444, -1, -1, -1, -1, -1, -1},
  {-1, 2949119, -1, -1, -1, -1, -1, -1, -1, -1, -65533, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 2949183, -1, -1, -1, -1, -1, -1, 5373951, 6750207, -1, -65474, -1, -1, -1, -1, -1, -1},
  {2949266, -65383, 4325375, -1, -1, -1, -1, -1, -1, -1, -1, 4259839, -1, -1, -1, -1, -1, -1, -1},
  {7405611, 9568297, -1, -1, -1, -1, -1, -1, -1, 10485759, 9568339, -65450, 10354687, -65536, -65534, -1, -1, -1, -1},
  {-1, -1, 10027007, -1, -1, -1, -1, -1, -1, 4784127, 3276836, 327750, -65525, -1, -1, -1, -1, -1, -1},
  {393309, 2359365, -1, -1, -1, -1, -1, -1, -1, -1, -65417, -1, -1, -1, -1, -1, -1, -1, -1},
  {4587519, 852087, -1, -1, -1, -1, -1, -1, -1, 2097151, 9830427, 9568289, -65490, -1, -65501, -1, -1, -1, -1},
  {-65439, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65393, -1, -1, -1},
  {-1, 6094847, -1, -1, -1, -1, -1, -1, -1, 9633791, 3211359, -65388, 524354, 262191, -65497, -1, -1, -1, -1},
  {-1, 5242879, -1, -1, -1, -1, -1, -1, -1, -1, 5111821, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, 7077887, -1, -1, -1, -1, -1, -1, -1, 6225919, 2949183, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 131071, -1, -1, -1, -1, -1, -1, 4456447, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-65451, -65515, -1, -1, -1, -1, -1, -1, -1, 2949119, -65452, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65469, -1, -1, -1, -1, -1, -1, -1, -1, -65533, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, 10158079, -1, -1, -1, 3014655, 2490526, -65378, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65390, -1, -1, -1, -1, -1, -1, -1, -65390, -1, -65485, -1, -1, -1, -1, -1, -1, -1},
  {7143433, 6750284, 9568315, -1, -1, -1, -1, -1, -1, -1, -65466, -1, -1, -1, -1, -1, -1, -1, -1},
  {5373951, -65419, -1, -1, -1, -1, -1, -1, -1, 5111807, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, 9437183, -1, -1, -1, -1, -1, -1, -1, 7012351, 7602250, -65390, -1, -1, -1, -1, -1, -1, -1},
  {-1, 6553699, -1, -1, -1, -1, -1, -1, -1, -1, 1507327, -65438, -1, -1, -1, -1, -1, -1, -1},
  {-1, 65634, -1, -1, -1, -1, -1, -65478, -1, -1, 2490367, -1, -1, -1, -1, -1, -1, -1, -1},
  {1507327, 4194303, -1, -1, -1, -1, -1, -1, -1, -1, 6094847, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 2490367, -1, -1, -1, -1, -1, -1, 7077887, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {6422527, -1, -1, -1, -1, -1, -1, -1, -1, 2162687, 5242985, -65422, -1, -65390, -65394, -1, -1, -1, -1},
  {1179647, -1, -1, -1, -1, -1, -1, -1, -1, 6094847, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-65393, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65519, -1, -1, -1, -1, -1, -1},
  {-1, 2490369, -1, -1, -1, -1, -1, -1, -1, 4390911, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65429, -1, -1, -1, -1, -1, -1, -1, 10420223, 2490419, -1, -65512, -1, -1, -1, -1, -1, -1},
  {-1, 6422527, -1, -1, -1, -1, -1, -1, -1, 7798783, -65491, -65442, -1, -1, -1, -1, -1, -1, -1},
  {7733247, -1, -1, -1, -1, -1, -1, -1, -1, 4718591, -1, -65464, -1, -1, -1, -1, -1, -1, -1},
  {-1, 7929978, -1, -1, -1, -1, -1, -1, -1, 3014655, 7929855, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, 7929855, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65506, -1, -1, -1, -1, -1, -1, -1},
  {-65399, 8519808, -1, -1, -1, -1, -1, -1, -1, 8650751, -1, -65409, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -65406, -1, -1, -1, -1, -1, -1, 8585215, 8192128, 9044100, -1, -1, -1, -1, -1, -1, -1},
  {10027135, 8126601, -1, -1, -1, -1, -1, -1, -1, 8781823, 8781959, -65400, -65398, -1, -1, -1, -1, -1, -1},
  {8388607, -1, 8323071, -1, -1, -1, -1, -1, -1, -1, -65411, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65409, -1, -1, -1, -1, -1, -1, -1, -1, -65407, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, 8519679, -1, -1, -1, -1, -1, -1, -1, 8519679, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -65407, -1, -1, -1, -1, -1, -1, -1, -1, -65412, -65408, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 8323201, -1, -1, -1, -1, -1, -1, -1, -1, -65396, -1, -1, -1, -1, -1, -1, -1},
  {-65395, -65397, -1, -1, 10158079, -1, -1, -1, -1, -1, -65396, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -65429, -1, -1, -1, -1, -1, 9633791, 6291566, 655385, -1, -1, 4980735, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, 1179647, -1, -1, -1, -1, -65510, -1, -1, -1, -1, -1, -1, -1, -1},
  {5046336, 4390929, 4784220, 6357018, 2424939, -65393, -1, 3211263, 5898282, -1, -1, -1, -1, -1, -65519, -1, -1, -1, -1},
  {-1, -65459, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65515, -1, -1, -1, -1, -1, -1, -1},
  {-1, 4849663, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1179647, -1, -1, -1, -1, -1, -1, -1},
  {-65382, -1, -65468, -1, -1, -1, -1, -1, -1, 8454143, 10158079, -1, -1, -1, -65491, -1, -1, -1, -1},
  {9961618, 10027022, -1, 2621580, -65484, -65448, -1, -1, -1, -1, -1, -65519, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, 1310719, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -65469, -1, -1, -1, -1},
  {7536729, -65486, 3801087, -1, -1, -1, -1, -1, -1, -1, -65469, -1, -1, -1, -1, -1, -1, -1, -1},
  {-65506, -1, -1, -1, -1, -1, -1, -1, -1, 65535, 0, 0, 0, 0, 0, 0, 1074120192, 1, 1074122734},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074122696, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074125564, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074122707, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074122714, 1, 1074120192, 1, 1074120192, 1, 1074125564, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074122741, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192},
  {1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074126002, 1, 1074125864, 1, 1074120192, 1, 1074120192, 1},
  {1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074122774, 1, 1074120192},
  {1, 1074126266, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 1074120192, 1, 0, 0, 0, 0},
  {0, 0, 0, 16777472, 16777216, 0, 0, 16843009, 16777473, 16777216, 16843008, 0, 0, 65536, 0, 0, 16777472, 0, 256},
  {0, 256, 0, 0, 1, 0, 256, 0, 0, 0, 0, 256, 0, 0, 65537, 16777216, 1, 0, 0},
  {0, 16842752, 0, 0, 1819232256, 544698220, 1734633810, 101, 0, 0, 0, 0, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1},
  {1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657},
  {1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1074174657, 1, 1864396353, 1663067244, 1852400225, 1634235424, 1635197044, 1852776563, 1965057379, 543450483},
  {1746958690, 1702129269, 539915122, 1931965513, 1701143072, 1650532462, 1868852833, 543450478, 544370534, 1701670771, 1835627552, 1646275685, 1948284021, 1931502952, 1668641396, 1701999988, 544434464, 1818850419, 1869815916},
  {778333813, 1936607520, 744842345, 1970239776, 1851876128, 1852401184, 1819222116, 1902452836, 1836083573, 745827941, 1920296480, 1970563438, 539780466, 543452769, 1752327536, 544436321, 1701670771, 1919903264, 1953787751},
  {1931505253, 1819308149, 779314537, 1701336096, 1650549536, 1746955881, 1629516641, 1937337632, 1769104756, 745764207, 1852795936, 544828517, 1869444193, 1701343347, 3040626, 0, 1864396353, 1998611564, 1919251553},
  {1818848544, 1752440940, 1746957409, 1814066017, 543649391, 1668180339, 1634082917, 1852140652, 1953392928, 1768169583, 1885696627, 779250017, 1701336096, 1935764768, 1702259059, 1869575968, 544105828, 1702125943, 1752637554},
  {543974757, 1830843241, 1869182063, 1936026734, 1663052915, 1684368225, 1953068832, 1970085992, 1851859044, 1818304612, 778396007, 1701336096, 1818848544, 1969365100, 1768189033, 1763731310, 1818588020, 1634213990, 1868767347},
  {1885432940, 543450483, 1881173609, 1701011820, 1914711155, 1634039397, 1735289196, 1684827936, 1667329312, 1701734760, 1763735922, 1684632430, 1226845797, 1746958710, 1663071073, 1835622764, 1830839397, 543712117, 1948280431},
  {1931502952, 1701736308, 1920234272, 1970561909, 539780466, 544503138, 1634476129, 1919247460, 1769239328, 1881173100, 1769369458, 544433508, 1701012321, 1948283763, 1752440943, 1886724197, 544367984, 1702258028, 1092628076},
  {1718968864, 1948265574, 1751610735, 1869573165, 1735289195, 1818848544, 544367980, 1802661751, 1752440947, 1634541669, 1852401763, 544830053, 1948285282, 1696621928, 1634890862, 778396526, 0, 1629515329, 1701405550},
  {1931506798, 1701736308, 1952540448, 1752440933, 1931506785, 1684955508, 1952522355, 1701344288, 1734632736, 1718558821, 1701344288, 1919903264, 779383653, 1701336096, 1952540448, 1936269413, 1987011360, 1684370021, 544106784},
  {1936945005, 1684955424, 1918984992, 1735289206, 1752440947, 1629516897, 1830839666, 1819571055, 1870078073, 1629515378, 544825719, 1948285282, 778399081, 661932320, 1853169779, 1634036835, 1752637554, 1948284001, 1730176360},
  {543519841, 544432503, 1851876717, 1869881460, 1635084064, 1864393842, 1634541682, 539782002, 544503138, 1746957417, 1629516641, 1851859054, 1852139875, 1881156724, 1919252335, 543978854, 1936028272, 1701015141, 46},
  {0, 1634934849, 1684370019, 1869768480, 1998611830, 1701995880, 1701344288, 1684827936, 544502629, 1701147252, 1953702003, 543452769, 1629515369, 1919509280, 778398819, 1701336096, 1919508768, 1919248416, 1701191781},
  {544435301, 1717987684, 1852142181, 539828340, 1918986339, 543450471, 1752459639, 1668178208, 1953391977, 1701733664, 779708274, 1931493664, 1701736308, 1953259808, 1931506273, 544437353, 1948282473, 1663067496, 1702129253},
  {1663052914, 1919252079, 1763730533, 1869422702, 1629516659, 1931502702, 1851880052, 1931502951, 1868721529, 539915116, 543516756, 1987015271, 1936269413, 1769304352, 539784293, 1869442145, 1914729587, 1919252069, 745827941},
  {544432416, 1746953833, 1768189039, 1763731310, 1646293876, 1952540018, 1495281256, 1713403247, 543974757, 1701538156, 1970239776, 543519271, 1851880563, 1735289188, 544106784, 1819287649, 543515489, 1730176623, 1952540018},
  {1734964000, 1768319342, 1668178275, 1092628069, 1918987808, 746024818, 1835819296, 544502639, 1769369193, 1818388851, 1634738277, 1814063220, 1935958373, 1919905312, 1763731572, 544175214, 1885693284, 1931506277, 1868849512},
  {3044215, 0, 2037194817, 1919251571, 1937076073, 1635021600, 1679846508, 1768710501, 1763731310, 1851859054, 1852139875, 1918967924, 1634101620, 745763939, 1734438176, 1818321769, 1702127904, 539784045, 543452769},
  {1701994866, 1920295712, 1769172841, 1936025972, 1750343726, 1920213093, 1919247457, 544434464, 2003791467, 1734632812, 1818386789, 1650532453, 544503151, 543516788, 1953720680, 544830063, 543452769, 1886351984, 1769239141},
  {1864397669, 1634017382, 1763731555, 778921332, 1836012320, 1953046629, 544435557, 2003790951, 1953068832, 1701978216, 1969514867, 1830841441, 1667852129, 1752637484, 543517801, 1701344367, 1629516658, 1931502962, 1819307369},
  {1700929657, 1769239905, 543978854, 1746956911, 1869902697, 1633905010, 544828524, 1852270963, 1667851881, 779382369, 0, 543516756, 1769239649, 1936269411, 1818846752, 543450476, 1752459639, 1684827936, 1920296480},
  {1970563438, 1663067506, 1919252079, 1763730533, 1969496174, 1931506803, 1952802152, 1092628083, 1634562848, 1663069292, 1953719656, 1953067808, 1852383347, 1701344288, 1919902496, 745694574, 1684955424, 1651467040, 1935828343},
  {1851877408, 1919295591, 1948282223, 1914725736, 1702127201, 3044210, 0, 1886593089, 1869177697, 1646293877, 2037080929, 543453793, 1768449378, 1948279918, 1746953576, 1702065519, 1984897070, 1919382117, 544110447},
  {1935766119, 1851859059, 1769414756, 1818649708, 1919252335, 1868767347, 544367990, 543516788, 1634038369, 1851858988, 543236196, 1852798067, 1634738277, 1814063220, 1935958373, 544175136, 1836261473, 543976545, 1684367475},
  {1849761838, 1684827936, 1769435936, 1746954094, 1936158305, 1869768224, 543236205, 1701147252, 1634886176, 745038702, 1684955424, 1981833504, 1952802661, 1701601889, 1918986016, 544105828, 1953459312, 1701407776, 1634082931},
  {2003790956, 1750343726, 1918967909, 1713398117, 1936483685, 1634037792, 1969644899, 1969365100, 1701716084, 1667591271, 778331508, 0, 1836261441, 543976545, 1685024631, 1931505253, 543450472, 1819044198, 1998611557},
  {543716457, 1685217639, 1852403301, 1869881447, 544435311, 543452769, 1886418291, 1936025964, 1632772142, 745760107, 1869116192, 1936483702, 1851858988, 1953439844, 544367976, 1819307369, 1852140901, 1746957172, 543649377},
  {1836020326, 1701344288, 1818326816, 539915116, 543516756, 544368993, 1818586483, 1864397676, 1634017382, 543716466, 543452769, 543452271, 1685024631, 541138990, 1818324339, 1769414764, 2003788910, 1952803872, 1852383347},
  {1835623456, 1734962208, 3044456, 1869422657, 1701603682, 1801544224, 544830053, 1953653091, 1717989152, 1852404325, 1919295591, 543716197, 1634038370, 1881156708, 1920234337, 745760105, 1684955424, 1702327072, 1948284005},
  {1952540018, 1411395187, 1629513064, 1634561906, 543584032, 1768644962, 1646290798, 1684104562, 544434464, 1701999209, 1953720691, 1701601897, 1750343726, 1633820773, 544367979, 1802661751, 1919295603, 1696623983, 2037150305},
  {1919905056, 1735289198, 1852121132, 1769108851, 1696622446, 2037540214, 1852401780, 1936269415, 1701996064, 1629513843, 1679844462, 1667853413, 1937076073, 46, 0, 1633820737, 1852793708, 1752440953, 1864397921},
  {1919247974, 543236211, 1853191283, 1735289198, 1701410336, 1718558839, 1819232288, 544698220, 1734633810, 1176514149, 544042866, 1701995880, 1870209068, 1633886325, 1702043758, 1752440933, 1852121189, 1701996916, 1852402464},
  {1836016743, 1919972128, 543449445, 544503151, 1869374818, 539828343, 543516788, 1953718627, 539780460, 1802658157, 1819309157, 744842081, 2003792928, 1629498478, 1931502702, 1869771381, 1768189557, 1629513582, 1935762802},
  {1750343726, 1769349221, 1763735397, 1936007283, 1768121712, 2037148769, 1634034208, 1718187125, 1629514869, 1970479220, 1936290414, 1851859045, 1970479204, 1952805742, 46, 0, 1953701953, 543516513, 1919248503},
  {1633820773, 544433266, 543452769, 1769174381, 1851877731, 1701847155, 1919903346, 1868963949, 1752440946, 1635000421, 1852990838, 1881174823, 1869771873, 539915118, 543516756, 1734440051, 1936269413, 1818588960, 1701064044},
  {1852270963, 1713398885, 1629516399, 1937076067, 1935894900, 1851858988, 1701847140, 1919903346, 1668178285, 1746957157, 543519333, 543519329, 1635216481, 1881174905, 1819635823, 539914849, 1701601620, 1684370542, 1937075488},
  {1634296681, 1713402734, 544042866, 1918985582, 1684955424, 1918985760, 1836016416, 1869881445, 1919250464, 1836216166, 544108320, 1936287860, 1635021600, 3040615, 0, 543516756, 1702060386, 1953391981, 544434464},
  {1802658148, 1684955424, 1835099168, 1461726832, 1919251553, 1769104416, 1713402736, 544042866, 1701670771, 1919248503, 1852383333, 1701344288, 1918985248, 1936027243, 1629498483, 1948279918, 1629513064, 1763734121, 1701322867},
  {544831073, 1752459639, 1701344288, 1701671712, 1864395884, 1634017382, 543716466, 543452769, 1633903972, 11897, 0, 1752375361, 1684370017, 1952539168, 1869574760, 1769414765, 1646291060, 1667855201, 1701667104},
  {1769236846, 539915109, 1769152577, 539781998, 1818849140, 539784293, 543452769, 2003789939, 1931506277, 1819042164, 1667460896, 544829557, 543516788, 1818324339, 1886593132, 778396513, 1701336096, 1869574688, 1936269421},
  {1853187616, 1869182051, 543973742, 544503138, 2003789939, 1953046643, 1734418547, 1998597221, 543716457, 1852993399, 2020173344, 1701999988, 1851859059, 1634082916, 543450468, 1701603700, 11891, 1634607169, 2003792498},
  {1818322976, 2036430700, 1634036768, 1735289188, 544175136, 543516788, 1919182178, 1936551791, 1866735662, 544436847, 1701734764, 1953456672, 1769152616, 745760100, 1684955424, 1998610720, 1868852841, 1952522359, 1701344288},
  {1684956448, 1869573152, 1864397675, 1864397941, 544367990, 543516788, 1685217639, 539913829, 543516756, 1819042152, 544825719, 1897952105, 1952803189, 1769414700, 1864394868, 544828526, 543516788, 1853190003, 1718558820},
  {1970239776, 1868963954, 1953723503, 544436325, 1948282479, 1998611816, 1701080943, 1818632302, 779251567, 0, 1869094977, 1931488372, 2037084013, 1919903264, 1713399143, 1701604457, 1769414756, 1948280948, 1931502952},
};

static const char *const WORLD_ITEM_LISTS_REC_000[] = {
  "abandoned_furniture",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_001[] = {
  "water_wheel",
  "rusty_gears",
  "ladder",
  "rope_pulley",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_002[] = {
  "ancient_gate",
  "stone_carvings",
  "moss_covered_stone",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_003[] = {
  "stone_altar",
  "ancient_symbols",
  "moss",
  "standing_stones",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_004[] = {
  "ancient_artifacts",
  "magical_items",
  "rare_curiosities",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_005[] = {
  "chest",
  "dust_sheet",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_006[] = {
  "swing",
  "vegetable_garden",
  "stone_path",
  "wildflowers",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_007[] = {
  "rake",
  "shovel",
  "gardening_tools",
  "seeds",
  "pots",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_008[] = {
  "fresh_bread",
  "pastries",
  "sweet_treats",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_009[] = {
  "balcony",
  "kingdom_view",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_010[] = {
  "performance_stage",
  "musical_instruments",
  "acoustic_design",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_011[] = {
  "old_box",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_012[] = {
  "sink",
  "toilet",
  "shower",
  "towel_rack",
  "medicine_cabinet",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_013[] = {
  "hallway_window",
  "door_frames",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_014[] = {
  "hammer",
  "tongs",
  "water_trough",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_015[] = {
  "smithing_tools",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_016[] = {
  "waystone_monolith",
  "elysium_crystal",
  "glowing_runes",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_017[] = {
  "royal_banner",
  "castle_gate",
  "guard_post",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_018[] = {
  "cooking_hearths",
  "copper_pots",
  "preparation_tables",
  "fresh_ingredients",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_019[] = {
  "watchtower",
  "battlements",
  "signal_flags",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_020[] = {
  "ancient_markings",
  "more_crystals",
  "hidden_chamber",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_021[] = {
  "glowing_crystal",
  "ancient_map",
  "mineral_ore",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_022[] = {
  "overgrown_vegetation",
  "cave_opening",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_023[] = {
  "stalactites",
  "water_pool",
  "glowing_crystals",
  "stone_formations",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_024[] = {
  "wine_bottle",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_025[] = {
  "cellar_tunnel",
  "dim_lighting",
  "mysterious_passage",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_026[] = {
  "administrative_buildings",
  "official_notices",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_027[] = {
  "rubble",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_028[] = {
  "notice_boards",
  "announcements",
  "job_postings",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_029[] = {
  "private_booths",
  "comfortable_seating",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_030[] = {
  "workshops",
  "craft_studios",
  "tool_sounds",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_031[] = {
  "glowing_crystals",
  "burial_niches",
  "crystal_embedded_walls",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_032[] = {
  "crystal_pond",
  "koi_fish",
  "water_lilies",
  "pond_bridge",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_033[] = {
  "dark_symbols",
  "ritual_markers",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_034[] = {
  "glowing_mushrooms",
  "moss",
  "fallen_leaves",
  "ancient_tree_bark",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_035[] = {
  "sanctum_chamber",
  "glowing_symbols",
  "ancient_carvings",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_036[] = {
  "dining_table",
  "chairs",
  "place_settings",
  "chandelier",
  "sideboard",
  "silverware",
  "portraits",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_037[] = {
  "coffee_bar",
  "fresh_roasts",
  "cozy_atmosphere",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_038[] = {
  "shed_door",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_039[] = {
  "pet_cages",
  "exotic_animals",
  "pet_supplies",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_040[] = {
  "wheat_field",
  "corn_field",
  "chicken_coop",
  "cow_pen",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_041[] = {
  "ferry_boat",
  "ferryman_station",
  "waiting_area",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_042[] = {
  "performance_stage",
  "festival_decorations",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_043[] = {
  "fishing_piers",
  "fishing_equipment",
  "fish_baskets",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_044[] = {
  "altar_stone",
  "journal",
  "mysterious_gem",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_045[] = {
  "stone_marker",
  "fallen_branch",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_046[] = {
  "ancient_weapons",
  "old_armor",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_047[] = {
  "crystal_ball",
  "tarot_cards",
  "mystical_symbols",
  "candles",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_048[] = {
  "magical_fountain",
  "floating_rings",
  "cascading_water",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_049[] = {
  "fresh_fruits",
  "vegetables",
  "produce_baskets",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_050[] = {
  "door",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_051[] = {
  "fountain",
  "herbs",
  "house_key",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_052[] = {
  "store_counter",
  "shelves",
  "barrels",
  "display_case",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_053[] = {
  "guard_station",
  "weapon_racks",
  "report_desk",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_054[] = {
  "bed",
  "desk",
  "chair",
  "window",
  "wardrobe",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_055[] = {
  "ancient_tome",
  "herb_bundle",
  "mysterious_artifact",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_056[] = {
  "hidden_alcove",
  "private_seating",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_057[] = {
  "old_wine",
  "preserved_foods",
  "dusty_jars",
  "ancient_tome",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_058[] = {
  "ancient_temple_ruins",
  "crystal_pool",
  "carved_stone",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_059[] = {
  "hidden_panel",
  "secret_space",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_060[] = {
  "rare_herbs",
  "glowing_mushrooms",
  "ancient_tree",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_061[] = {
  "stone_altar",
  "ancient_pillars",
  "wildflowers",
  "stream",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_062[] = {
  "path_markers",
  "wildflowers",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_063[] = {
  "boulder",
  "wild_grasses",
  "viewpoint",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_064[] = {
  "weathered_stone",
  "ancient_carving",
  "crystal_fragment",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_065[] = {
  "cave_crystal",
  "ancient_symbol",
  "clear_water",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_066[] = {
  "docks",
  "boats",
  "fishing_equipment",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_067[] = {
  "ancient_trees",
  "forest_paths",
  "mystical_atmosphere",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_068[] = {
  "bed",
  "table",
  "window",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_069[] = {
  "table",
  "book",
  "lantern",
  "coat_rack",
  "mirror",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_070[] = {
  "stove",
  "sink",
  "cupboards",
  "butcher_block",
  "drying_herbs",
  "pots",
  "pans",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_071[] = {
  "bookshelves",
  "ladder",
  "reading_lamp",
  "open_books",
  "reading_nook",
  "linguistics_textbook",
  "lore_scroll",
  "crafting_manual",
  "armor_crafting_manual",
  "advanced_crafting_tome",
  "herbalism_guide",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_072[] = {
  "armchair",
  "coffee_table",
  "vase",
  "bookshelf",
  "fireplace",
  "old_rug",
  "curtains",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_073[] = {
  "cave_entrance",
  "ancient_passages",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_074[] = {
  "apartment_buildings",
  "ground_floor_shops",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_075[] = {
  "main_tables",
  "tavern_bar",
  "fireplace",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_076[] = {
  "elegant_furniture",
  "large_windows",
  "tasteful_decorations",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_077[] = {
  "market_stalls",
  "colorful_awnings",
  "merchant_carts",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_078[] = {
  "bathtub",
  "sink",
  "mirror",
  "towel_rack",
  "medicine_cabinet",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_079[] = {
  "four_poster_bed",
  "bed_curtains",
  "wardrobe",
  "vanity_table",
  "personal_items",
  "dresser",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_080[] = {
  "hedge_maze",
  "reflection_mirrors",
  "contemplation_benches",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_081[] = {
  "wildflowers",
  "butterflies",
  "tall_grass",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_082[] = {
  "miller_journal",
  "engineering_tools",
  "rope_pulley",
  "workbench",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_083[] = {
  "moonlight_clearing",
  "silvery_light",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_084[] = {
  "cave_painting",
  "mineral_crystals",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_085[] = {
  "ancient_carvings",
  "tinderbox",
  "obsidian_shard",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_086[] = {
  "mushroom_circle",
  "glowing_mushrooms",
  "magical_energy",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_087[] = {
  "nexus_monolith",
  "dimensional_rune",
  "energy_crystal",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_088[] = {
  "nexus_monolith",
  "elysium_crystal",
  "dimensional_runes",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_090[] = {
  "notice_board",
  "announcements",
  "job_postings",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_091[] = {
  "well_bucket",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_092[] = {
  "luxury_apartment",
  "premium_accommodations",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_093[] = {
  "empty_jars",
  "preserved_foods",
  "storage_containers",
  "shelves",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_094[] = {
  "water_lilies",
  "reeds",
  "wooden_dock",
  "fishing_spot",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_095[] = {
  "potions",
  "elixirs",
  "alchemical_ingredients",
  "glass_bottles",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_096[] = {
  "private_rooms",
  "comfortable_beds",
  "traveler_accommodations",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_097[] = {
  "residential_homes",
  "community_gardens",
  "street_lamps",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_098[] = {
  "stone_pillars",
  "broken_rope_bridge",
  "river_treasure",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_099[] = {
  "makeshift_raft",
  "rope",
  "vines",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_100[] = {
  "rowboat",
  "wooden_dock",
  "fishing_hook",
  "river_stones",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_101[] = {
  "coffee_beans",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_102[] = {
  "boulders",
  "lichen",
  "moss",
  "rock_crevices",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_103[] = {
  "rooftop",
  "panoramic_views",
  "rooftop_garden",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_104[] = {
  "rooftop_seating",
  "morning_views",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_105[] = {
  "rose_archway",
  "climbing_roses",
  "garden_tunnel",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_106[] = {
  "connected_houses",
  "decorative_gardens",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_107[] = {
  "garden_paths",
  "flowering_plants",
  "landscaped_areas",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_108[] = {
  "ancient_tomes",
  "scrolls",
  "reading_chairs",
  "fireplace",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_109[] = {
  "sealed_door",
  "mysterious_room",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_110[] = {
  "secret_room",
  "private_meeting_space",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_111[] = {
  "simple_beds",
  "personal_belongings",
  "storage_chests",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_112[] = {
  "tools",
  "rope",
  "tension_wrench",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_113[] = {
  "hidden_nooks",
  "shadowy_corners",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_114[] = {
  "songbird_trees",
  "bird_songs",
  "grove_benches",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_115[] = {
  "gate",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_116[] = {
  "stone_bridge",
  "quality_homes",
  "stream",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_117[] = {
  "smooth_stones",
  "stream_water",
  "fish",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_118[] = {
  "natural_spring",
  "waterfall",
  "ferns",
  "moss",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_119[] = {
  "desk",
  "inkwell",
  "quill",
  "papers",
  "globe",
  "reading_chair",
  "old_books",
  "engineering_tome",
  "merchant_ledger",
  "fine_lockpick",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_120[] = {
  "rotten_plank",
  "marsh_gas",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_121[] = {
  "bog_fungi",
  "twisted_root",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_122[] = {
  "ancient_tree",
  "mysterious_shovel",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_123[] = {
  "fabrics",
  "finished_garments",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_124[] = {
  "barrels",
  "crates",
  "delivery_door",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_125[] = {
  "comfortable_chairs",
  "old_maps",
  "nautical_artifacts",
  "candles",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_126[] = {
  "ale_barrel",
  "wine_barrel",
  "preserved_foods",
  "pickled_vegetables",
  "dried_meat",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_127[] = {
  "bar",
  "barstool",
  "mug",
  "fireplace",
  "tables",
  "chairs",
  "tavern_piano",
  "ale",
  "mead",
  "wine",
  "roasted_meat",
  "bread",
  "cheese",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_128[] = {
  "tavern_sign",
  "hitching_post",
  "lantern",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_129[] = {
  "hallway_window",
  "number_plates",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_130[] = {
  "hearth",
  "cooking_pot",
  "spit",
  "prep_table",
  "flour_barrel",
  "salt_barrel",
  "stew",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_131[] = {
  "private_table",
  "chairs",
  "lantern",
  "locked_chest",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_132[] = {
  "privy_seat",
  "latch",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_133[] = {
  "bed",
  "straw_mattress",
  "table",
  "chair",
  "window",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_134[] = {
  "bed",
  "washbasin",
  "chest",
  "window",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_135[] = {
  "bed",
  "comfortable_mattress",
  "writing_desk",
  "window",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_136[] = {
  "bed",
  "wardrobe",
  "table",
  "balcony",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_137[] = {
  "horse_stalls",
  "hay",
  "water_trough",
  "tack",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_138[] = {
  "portraits",
  "tapestries",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_139[] = {
  "flowers",
  "fountain",
  "meditation_bench",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_140[] = {
  "altar",
  "incense_burner",
  "prayer_cushions",
  "holy_symbols",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_141[] = {
  "weapon_rack",
  "training_dummy",
  "practice_weapons",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_142[] = {
  "hidden_bridge",
  "overhanging_plants",
  "secluded_area",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_143[] = {
  "tables",
  "warm_atmosphere",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_144[] = {
  "royal_throne",
  "ornate_tapestries",
  "stained_glass_windows",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_145[] = {
  "meeting_hall",
  "official_documents",
  "public_forum",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_146[] = {
  "fountain_of_the_sky_rings",
  "festival_stage",
  "notice_board",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_147[] = {
  "practice_dummies",
  "weapon_racks",
  "training_equipment",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_148[] = {
  "exotic_goods",
  "rare_items",
  "traveling_merchant_stalls",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_149[] = {
  "treasure",
  "ancient_scroll",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_150[] = {
  "underground_lake",
  "dark_water",
  "small_boat",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_151[] = {
  "ancient_artifacts",
  "iron_doors",
  "torches",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_152[] = {
  "fireplace",
  "inn_table",
  "inn_chair",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_153[] = {
  "signpost",
  "roadside_stone",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_154[] = {
  "well",
  "market_stall",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_155[] = {
  "war_table",
  "tactical_maps",
  "battle_standards",
  "weapon_display",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_156[] = {
  "spyglass",
  "signal_fire_pit",
  "observation_table",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_157[] = {
  "waterfall",
  "echoing_sounds",
  "pool",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_158[] = {
  "mailbox",
  "scrap_metal",
  "wood_scrap",
  "lockpick",
  "rusty_pick",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_159[] = {
  "whispering_trees",
  "mystical_path",
  NULL
};
static const char *const WORLD_ITEM_LISTS_REC_160[] = {
  "furniture",
  "wood_supplies",
  NULL
};
static const char *const *const WORLD_ITEM_LISTS_REC[WORLD_ROOM_COUNT] = {
  WORLD_ITEM_LISTS_REC_000,
  WORLD_ITEM_LISTS_REC_001,
  WORLD_ITEM_LISTS_REC_002,
  WORLD_ITEM_LISTS_REC_003,
  WORLD_ITEM_LISTS_REC_004,
  WORLD_ITEM_LISTS_REC_005,
  WORLD_ITEM_LISTS_REC_006,
  WORLD_ITEM_LISTS_REC_007,
  WORLD_ITEM_LISTS_REC_008,
  WORLD_ITEM_LISTS_REC_009,
  WORLD_ITEM_LISTS_REC_010,
  WORLD_ITEM_LISTS_REC_011,
  WORLD_ITEM_LISTS_REC_012,
  WORLD_ITEM_LISTS_REC_013,
  WORLD_ITEM_LISTS_REC_014,
  WORLD_ITEM_LISTS_REC_015,
  WORLD_ITEM_LISTS_REC_016,
  WORLD_ITEM_LISTS_REC_017,
  WORLD_ITEM_LISTS_REC_018,
  WORLD_ITEM_LISTS_REC_019,
  WORLD_ITEM_LISTS_REC_020,
  WORLD_ITEM_LISTS_REC_021,
  WORLD_ITEM_LISTS_REC_022,
  WORLD_ITEM_LISTS_REC_023,
  WORLD_ITEM_LISTS_REC_024,
  WORLD_ITEM_LISTS_REC_025,
  WORLD_ITEM_LISTS_REC_026,
  WORLD_ITEM_LISTS_REC_027,
  WORLD_ITEM_LISTS_REC_028,
  WORLD_ITEM_LISTS_REC_029,
  WORLD_ITEM_LISTS_REC_030,
  WORLD_ITEM_LISTS_REC_031,
  WORLD_ITEM_LISTS_REC_032,
  WORLD_ITEM_LISTS_REC_033,
  WORLD_ITEM_LISTS_REC_034,
  WORLD_ITEM_LISTS_REC_035,
  WORLD_ITEM_LISTS_REC_036,
  WORLD_ITEM_LISTS_REC_037,
  WORLD_ITEM_LISTS_REC_038,
  WORLD_ITEM_LISTS_REC_039,
  WORLD_ITEM_LISTS_REC_040,
  WORLD_ITEM_LISTS_REC_041,
  WORLD_ITEM_LISTS_REC_042,
  WORLD_ITEM_LISTS_REC_043,
  WORLD_ITEM_LISTS_REC_044,
  WORLD_ITEM_LISTS_REC_045,
  WORLD_ITEM_LISTS_REC_046,
  WORLD_ITEM_LISTS_REC_047,
  WORLD_ITEM_LISTS_REC_048,
  WORLD_ITEM_LISTS_REC_049,
  WORLD_ITEM_LISTS_REC_050,
  WORLD_ITEM_LISTS_REC_051,
  WORLD_ITEM_LISTS_REC_052,
  WORLD_ITEM_LISTS_REC_053,
  WORLD_ITEM_LISTS_REC_054,
  WORLD_ITEM_LISTS_REC_055,
  WORLD_ITEM_LISTS_REC_056,
  WORLD_ITEM_LISTS_REC_057,
  WORLD_ITEM_LISTS_REC_058,
  WORLD_ITEM_LISTS_REC_059,
  WORLD_ITEM_LISTS_REC_060,
  WORLD_ITEM_LISTS_REC_061,
  WORLD_ITEM_LISTS_REC_062,
  WORLD_ITEM_LISTS_REC_063,
  WORLD_ITEM_LISTS_REC_064,
  WORLD_ITEM_LISTS_REC_065,
  WORLD_ITEM_LISTS_REC_066,
  WORLD_ITEM_LISTS_REC_067,
  WORLD_ITEM_LISTS_REC_068,
  WORLD_ITEM_LISTS_REC_069,
  WORLD_ITEM_LISTS_REC_070,
  WORLD_ITEM_LISTS_REC_071,
  WORLD_ITEM_LISTS_REC_072,
  WORLD_ITEM_LISTS_REC_073,
  WORLD_ITEM_LISTS_REC_074,
  WORLD_ITEM_LISTS_REC_075,
  WORLD_ITEM_LISTS_REC_076,
  WORLD_ITEM_LISTS_REC_077,
  WORLD_ITEM_LISTS_REC_078,
  WORLD_ITEM_LISTS_REC_079,
  WORLD_ITEM_LISTS_REC_080,
  WORLD_ITEM_LISTS_REC_081,
  WORLD_ITEM_LISTS_REC_082,
  WORLD_ITEM_LISTS_REC_083,
  WORLD_ITEM_LISTS_REC_084,
  WORLD_ITEM_LISTS_REC_085,
  WORLD_ITEM_LISTS_REC_086,
  WORLD_ITEM_LISTS_REC_087,
  WORLD_ITEM_LISTS_REC_088,
  EMPTY_LIST,
  WORLD_ITEM_LISTS_REC_090,
  WORLD_ITEM_LISTS_REC_091,
  WORLD_ITEM_LISTS_REC_092,
  WORLD_ITEM_LISTS_REC_093,
  WORLD_ITEM_LISTS_REC_094,
  WORLD_ITEM_LISTS_REC_095,
  WORLD_ITEM_LISTS_REC_096,
  WORLD_ITEM_LISTS_REC_097,
  WORLD_ITEM_LISTS_REC_098,
  WORLD_ITEM_LISTS_REC_099,
  WORLD_ITEM_LISTS_REC_100,
  WORLD_ITEM_LISTS_REC_101,
  WORLD_ITEM_LISTS_REC_102,
  WORLD_ITEM_LISTS_REC_103,
  WORLD_ITEM_LISTS_REC_104,
  WORLD_ITEM_LISTS_REC_105,
  WORLD_ITEM_LISTS_REC_106,
  WORLD_ITEM_LISTS_REC_107,
  WORLD_ITEM_LISTS_REC_108,
  WORLD_ITEM_LISTS_REC_109,
  WORLD_ITEM_LISTS_REC_110,
  WORLD_ITEM_LISTS_REC_111,
  WORLD_ITEM_LISTS_REC_112,
  WORLD_ITEM_LISTS_REC_113,
  WORLD_ITEM_LISTS_REC_114,
  WORLD_ITEM_LISTS_REC_115,
  WORLD_ITEM_LISTS_REC_116,
  WORLD_ITEM_LISTS_REC_117,
  WORLD_ITEM_LISTS_REC_118,
  WORLD_ITEM_LISTS_REC_119,
  WORLD_ITEM_LISTS_REC_120,
  WORLD_ITEM_LISTS_REC_121,
  WORLD_ITEM_LISTS_REC_122,
  WORLD_ITEM_LISTS_REC_123,
  WORLD_ITEM_LISTS_REC_124,
  WORLD_ITEM_LISTS_REC_125,
  WORLD_ITEM_LISTS_REC_126,
  WORLD_ITEM_LISTS_REC_127,
  WORLD_ITEM_LISTS_REC_128,
  WORLD_ITEM_LISTS_REC_129,
  WORLD_ITEM_LISTS_REC_130,
  WORLD_ITEM_LISTS_REC_131,
  WORLD_ITEM_LISTS_REC_132,
  WORLD_ITEM_LISTS_REC_133,
  WORLD_ITEM_LISTS_REC_134,
  WORLD_ITEM_LISTS_REC_135,
  WORLD_ITEM_LISTS_REC_136,
  WORLD_ITEM_LISTS_REC_137,
  WORLD_ITEM_LISTS_REC_138,
  WORLD_ITEM_LISTS_REC_139,
  WORLD_ITEM_LISTS_REC_140,
  WORLD_ITEM_LISTS_REC_141,
  WORLD_ITEM_LISTS_REC_142,
  WORLD_ITEM_LISTS_REC_143,
  WORLD_ITEM_LISTS_REC_144,
  WORLD_ITEM_LISTS_REC_145,
  WORLD_ITEM_LISTS_REC_146,
  WORLD_ITEM_LISTS_REC_147,
  WORLD_ITEM_LISTS_REC_148,
  WORLD_ITEM_LISTS_REC_149,
  WORLD_ITEM_LISTS_REC_150,
  WORLD_ITEM_LISTS_REC_151,
  WORLD_ITEM_LISTS_REC_152,
  WORLD_ITEM_LISTS_REC_153,
  WORLD_ITEM_LISTS_REC_154,
  WORLD_ITEM_LISTS_REC_155,
  WORLD_ITEM_LISTS_REC_156,
  WORLD_ITEM_LISTS_REC_157,
  WORLD_ITEM_LISTS_REC_158,
  WORLD_ITEM_LISTS_REC_159,
  WORLD_ITEM_LISTS_REC_160
};

static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_003[] = {
  "grove_offering",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_005[] = {
  "loose_tile",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_019[] = {
  "hidden_scroll",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_021[] = {
  "crystal_heart",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_044[] = {
  "hollow_stump",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_061[] = {
  "shrine_offering",
  "ancient_prayer_beads",
  "skeleton_key",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_064[] = {
  "ridge_treasure",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_065[] = {
  "ancient_artifact",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_091[] = {
  "ancient_coin",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_112[] = {
  "loose_floorboard",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_122[] = {
  "buried_treasure",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_125[] = {
  "sheet_candlelight_reel",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_127[] = {
  "tavern_secret",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_139[] = {
  "sheet_last_call_etude",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_156[] = {
  "tower_logbook",
  NULL
};
static const char *const WORLD_HIDDEN_ITEM_LISTS_REC_158[] = {
  "hidden_cash",
  "buried_coin",
  NULL
};
static const char *const *const WORLD_HIDDEN_ITEM_LISTS_REC[WORLD_ROOM_COUNT] = {
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_003,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_005,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_019,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_021,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_044,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_061,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_064,
  WORLD_HIDDEN_ITEM_LISTS_REC_065,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_091,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_112,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_122,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_125,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_127,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_139,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_156,
  EMPTY_LIST,
  WORLD_HIDDEN_ITEM_LISTS_REC_158,
  EMPTY_LIST,
  EMPTY_LIST
};

static int ieq(const char *a, const char *b) {
  unsigned char ca, cb;
  if (!a || !b) return 0;
  while (*a && *b) { ca=(unsigned char)*a++; cb=(unsigned char)*b++; if (tolower(ca)!=tolower(cb)) return 0; }
  return *a=='\0' && *b=='\0';
}

const char *const *world_consume_food_ids(void) { return FOOD_IDS; }
const char *const *world_consume_drink_ids(void) { return DRINK_IDS; }
const char *const *world_quest_hints(void) { return QUEST_HINTS; }
const char *world_slug(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_SLUGS_REC[room] : "unknown"; }
const char *world_title(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_TITLES_REC[room] : "Unknown"; }
const char *world_blurb(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_BLURBS_REC[room] : ""; }
const char *world_region(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_REGIONS_REC[room] : ""; }
int world_room_is_dark(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? (int)WORLD_DARK_REC[room] : 0; }
const char *world_room_entity(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_ENTITIES_REC[room] : ""; }
int world_exit(int room, int dir) { if (room<0||room>=WORLD_ROOM_COUNT||dir<0||dir>=DIR_COUNT) return -1; return WORLD_EXITS_REC[room][dir]; }
const char *const *world_item_list(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_ITEM_LISTS_REC[room] : EMPTY_LIST; }
const char *const *world_hidden_item_list(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_HIDDEN_ITEM_LISTS_REC[room] : EMPTY_LIST; }
int world_room_index(const char *slug) { int i; if (!slug||!slug[0]) return -1; for (i=0;i<WORLD_ROOM_COUNT;i++) if (ieq(WORLD_SLUGS_REC[i], slug)) return i; return -1; }

static const char *const NPC_blacksmith_CHATTER[] = {
  "Gotta get the forge hot. Can't work cold metal.",
  "Everything in its place. Organization is key to good smithing.",
  "Good tools make good work. I keep mine in top condition.",
  "This is the real work. Heat, hammer, shape. Repeat until it's perfect.",
  "The rhythm of the hammer is like music. Once you find it, the work flows.",
  "The tempering process. This is what makes steel strong.",
  "I can fix most anything. Bring me your broken gear, and I'll make it good as new.",
  "A sharp blade is a safe blade. Dull weapons are dangerous.",
  "Armor needs care. Keep it maintained, and it'll keep you alive.",
  "Gotta keep track of what I have. Can't work without materials.",
  "Good stock of materials. Ready for whatever comes through that door.",
  "Always thinking about the next piece. A smith's work is never done.",
  "Almost done. Just need to make sure everything's perfect.",
  "Take care of your tools, and they'll take care of you.",
  "Time to let the forge cool down. It's been a good day's work.",
  "Got a few orders to fill. Good to stay busy.",
  "Even I need to rest sometimes. Hard work takes it out of you.",
  "Another day done. Tomorrow's another chance to make something great.",
  NULL
};
static const AetNpcTopic NPC_blacksmith_TOPICS[] = {
  {"apprentice learn teach training", "I've trained a few apprentices over the years. It's hard work, but rewarding. If you're interested in smithing, I can teach you the basics - though it takes years to master."},
  {"armor protection defense plate", "Armor's not just about stopping a blade - it's about mobility, comfort, and durability. I can craft or upgrade armor that'll keep you safe without weighing you down."},
  {"buy purchase want buy need buy looking to buy", "I've got materials for sale. What do you need?"},
  {"craft make forge create smith", "I've made everything from simple nails to masterwork blades. The key is patience, skill, and good materials. Bring me quality steel, and I'll make you something special."},
  {"fire forge fire heat flame", "The forge fire is the heart of my work. It needs to be hot enough to work steel, but controlled. Takes years to learn how to read the fire, know when it's ready."},
  {"forge this place workshop smithy", "This forge has been my life for twenty years. There's nothing like working with your hands, creating something from raw metal."},
  {"hello hi hey greetings good day", "Hey there! Welcome to my forge. Need something forged or repaired?"},
  {"help assist aid need help can you help", "Sure! I can upgrade your gear, repair damaged items, or sell you materials. What do you need?"},
  {"history past story tell me about", "I've been smithing since I was fifteen. Started as an apprentice to old Master Thorne. He taught me everything I know. When he passed, I took over this forge. Been here ever since."},
  {"materials steel iron ingot metal", "Good materials make good work. I always keep quality ingots in stock. Steel's better than iron for weapons, but iron works fine for basic repairs. Bring me rare materials, and I can do special upgrades."},
  {"repair fix mend restore", "I can repair damaged gear. What needs fixing?"},
  {"sell trade give you offer", "I'll buy scrap metal and broken gear. What've you got?"},
  {"thanks thank you appreciate grateful", "You're welcome! Always happy to help. Come back anytime you need smithing work done."},
  {"upgrade improve enhance make better strengthen", "I can upgrade your weapons and armor! What would you like me to work on?"},
  {"weapons sword blade weapon", "A good weapon is a balance of strength, sharpness, and weight. I can make you something that'll last a lifetime, or upgrade what you've got to be even better."},
  {"what do you have what can i buy what's for sale materials", "I've got ingots, leather, and tools. Let me show you..."},
  {NULL, NULL}
};
static const char *const NPC_farmer_CHATTER[] = {
  "He has the kind of beauty that comes from the sun and soil—broad shoulders softened by a loose linen shirt, and forearms corded with muscle from years of honest labor. His hair is messy and sun-bleached, and he smells like fresh rain and warm hay. There is a gentleness in his eyes that suggests he prefers listening to ",
  NULL
};
static const AetNpcTopic NPC_farmer_TOPICS[] = {
  {"animals livestock chickens cow milk eggs wool", "They trust me. That trust is heavy, but it feels good. To be responsible for another living thing... it changes you."},
  {"great job impressive well done you are reliable good farm strong", "Thank you. I don't get many compliments. Especially not from someone as lovely as you. I'll... I'll be thinking about that all day."},
  {"farm farming field soil crops harvest", "The soil here is rich. Dark and fertile. You have to treat it right, touch it gently, and it gives you everything it has. It's a beautiful relationship."},
  {"help assist advice what should i do hint stuck", "Don't rush. Slow down. Breathe. The answer usually comes when you stop forcing it. If you need a place to hide away and think, my door is open."},
  {"your name who are you what are you called introduce yourself", "I'm Jasper. Just a man who loves the land. And maybe... hopefully... someone to share it with one day."},
  {"seeds planting fertilizer tools plow", "Planting is the most important part. You have to bury the seed deep, keep it warm, and wait. The anticipation is half the fun."},
  {"thanks thank you appreciate grateful", "You don't have to thank me. Seeing you smile is plenty payment enough."},
  {"trade buy sell prices what do you have inventory", "I grew these myself. Picked the best ones just in case you stopped by. Take a look."},
  {"village people locals community home", "The village is noisy. I prefer it here. You can hear yourself think. You can hear yourself breathe. It's intimate."},
  {"weather rain storm drought season hot", "It's hot today. The kind of heat that sticks to your skin and makes your clothes heavy. I don't mind the sweat, though. Makes the water feel better later."},
  {"work job occupation what do you do chores", "I work with my hands. It's physical. Exhausting. But at the end of the day, when I wash the dirt off... I feel useful. Strong."},
  {NULL, NULL}
};
static const char *const NPC_forest_hermit_CHATTER[] = {
  "An ancient, mysterious figure who lives deep in the forest. Their age is impossible to determine, and their eyes hold the wisdom of countless years. They wear tattered robes that blend with the forest, and move with an almost supernatural silence. Their knowledge of ancient secrets and forgotten lore is vast, and they ",
  NULL
};
static const AetNpcTopic NPC_forest_hermit_TOPICS[] = {
  {NULL, NULL}
};
static const char *const NPC_general_store_owner_CHATTER[] = {
  "Everything has a place. Especially when you're working in the dark.",
  "We're low on silk rope and massage oil. People must be having a busy week.",
  "If it doesn't look inviting, they won't want to touch it. It's all about the presentation.",
  "I have exactly what you need. And maybe a few things you didn't know you wanted.",
  "Heavy lifting keeps the body in good shape. Wouldn't want to go soft, would I?",
  "Securing new routes. I like bringing new toys into the village.",
  "You hear the best secrets when people think you're just a simple shopkeeper.",
  "Competition is healthy. It keeps the blood pumping.",
  "Mm. Heat feels good. Almost makes me want to close up early.",
  "Money is just a score. The real game is the exchange.",
  "This is where I keep the inventory that requires a... private consultation.",
  "Thinking ahead. I always have a plan for what comes next.",
  "Almost time to dim the lights. That's when the real conversations happen.",
  "Ready for tomorrow. Or for someone who can't wait that long.",
  "Keep it clean, keep it smooth. Nobody likes a rough surface.",
  "The shop feels different at night. More private. You could get away with anything in here.",
  "I know exactly who owes me a favor. And I always collect.",
  "Long day. I'm just waiting for a reason to finally head upstairs. Care to give me one?",
  NULL
};
static const AetNpcTopic NPC_general_store_owner_TOPICS[] = {
  {"business customers sales trade", "Business is steady. People trust me because I know when to talk and when to keep my mouth shut. Most of the time."},
  {"buy purchase take get i'll take i want", "Excellent choice. You have a very... discerning eye. I like that."},
  {"nice good great wonderful amazing impressive beautiful handsome attractive kind helpful", "Flattery will get you everywhere with me. Especially if you keep looking at me with those hungry eyes."},
  {"flirt attractive beautiful handsome sexy hot cute lovely gorgeous", "Careful now. I've got a back room full of silk rope and no witnesses. Keep talking like that and I might have to take you into custody."},
  {"help assist need help looking for", "I'm very good with my hands and even better at solving problems. Tell me what's bothering you, and let's see if we can't work it out."},
  {"hobby interest what do you like enjoy passion", "I like organization, smooth silk, and a strong drink after the doors are locked. I also have a passion for... people-watching. You're particularly interesting to watch."},
  {"items what do you have what do you sell inventory wares goods what's available", "I have the practical stuff out front, and the pleasurable stuff in the back. Which one are you in the mood for today?"},
  {"price cost how much expensive cheap", "My prices are fair, but I don't give things away for free. Unless you've got something to trade that isn't made of gold."},
  {"sell trade give you offer have anything to sell", "Set it on the counter, honey. Let's see what you've brought me. I'm always looking for new things to play with."},
  {"store shop business inventory wares goods", "This shop is the heart of the village. I provide the light to see, the rope to climb, and the oil to... well, let's just say I have you covered."},
  {"thanks thank you appreciate grateful", "Don't thank me yet. You haven't seen the full service. Come back soon."},
  {"village town people community villagers", "It's a quiet little place, but everyone has their 'needs'. I'm just the one brave enough to stock the solution."},
  {"work working job occupation what do you do", "I run the shop. I keep the village provisioned and the travelers happy. It's a heavy load, but I've got the stamina for it."},
  {NULL, NULL}
};
static const char *const NPC_miller_CHATTER[] = {
  "Gotta check the stock. Only the best grain goes through my mill.",
  "Used to have chickens here. Still feed 'em when I can - keeps me in the habit.",
  "That wheel's been stuck for a while. I'll get it running again someday.",
  "These gears need work, but I know how to fix 'em. Just need the right parts.",
  "My dad's old journal. He taught me everything about this mill.",
  "Oil here, grease there... I'll get this thing running smooth again.",
  "River's got good fish. Nothing beats fresh catch after a hard day's work.",
  "Love watching this river. Never stops moving, just like me.",
  "Seen this river flood twice. '67 and '89. Both times I kept the mill running.",
  "Hand-grinding's slower than the wheel, but I get it done. Good workout too.",
  "Even I need a breather sometimes. Hard work builds character.",
  "Business is steady. Keeps me fed and the mill running.",
  "Got enough food here if you're hungry. I always cook extra.",
  "Best part of the day. Earned this rest after all that grinding.",
  "Fire's essential out here. Keeps me warm and fed.",
  "Keeping track of the day. Mill's been in the family for generations.",
  "Everything has its place. Learned that from my dad.",
  "Gonna get this place running like new. Just takes hard work and dedication.",
  NULL
};
static const AetNpcTopic NPC_miller_TOPICS[] = {
  {"what's your name who are you your name introduce yourself", "So what's your name, stranger?"},
  {"buy purchase want buy need buy looking to buy", "What do you need? I've got good stuff."},
  {"family parents siblings relatives", "My family's been running this mill for generations. I'm keeping the tradition alive, even if it's just me now."},
  {"hello hi hey greetings good day", "Hey there! What brings you to my mill?"},
  {"help assist aid need help can you help", "Sure, I can help! What do you need? I'm good with my hands and I know this area well."},
  {"how much price cost worth", "Prices are fair. I don't rip people off."},
  {"mill this place mill business", "This mill's been in my family for generations. I'm keeping it running, even if I have to do it all by hand."},
  {"name who are you what's your name introduce", "I'm Brenna, the miller. Most folks just call me Brenna and keep it simple."},
  {"river water stream", "Love this river. It's strong, steady, and never gives up. Reminds me of myself."},
  {"sell trade give you offer have anything to sell", "Sure, I'm always looking for materials. What've you got?"},
  {"thanks thank you appreciate grateful", "You're welcome! Always happy to help a friend."},
  {"water wheel wheel mill wheel gears", "That wheel? Been broken for a while. River shifted after a big storm. I'll fix it eventually - just need the right parts and some muscle."},
  {"weather rain storm sunny clouds", "Weather affects everything here. Rain means the river's stronger, which would help if that wheel worked. But I manage either way."},
  {"what do you have what can i buy what's for sale inventory wares goods", "Let me show you what I've got..."},
  {"work working job occupation what do you do", "I run this mill. It's hard work, but I love it. There's something satisfying about grinding grain and making flour."},
  {NULL, NULL}
};
static const char *const NPC_paladin_marcus_CHATTER[] = {
  "Control over the body is the highest form of discipline. Whether in combat, or in someone's bed.",
  "I pray for the strength to be a safe harbor in a dangerous world.",
  "I center myself so that when others lean on me, I do not break.",
  "Those who seek pleasure are welcome. Those who seek to take it by force will answer to me.",
  "There is no shame in tears. Let it out. I will hold the space for you.",
  "We are guardians, not butchers. Use your weight to stop the fight, not finish it.",
  "Intimacy requires privacy. I make sure our worshippers have it.",
  "Armor is heavy. Sometimes it is a relief to shed it and feel the air on my skin.",
  "My body is a shield. I train so it does not fail when it matters most.",
  "They offer the Architect's grace. I offer the Architect's steel. Both are necessary.",
  "Look at them. Beautiful. To see my people so free... it makes every battle worth it.",
  "If he touches you again when you say no, you come find me. Do you understand?",
  "Take a deep breath. You're safe now. I've got you.",
  "The dark is for rest and passion. I make sure neither is interrupted.",
  "Another day of safe harbor. The Architect smiles upon us.",
  NULL
};
static const AetNpcTopic NPC_paladin_marcus_TOPICS[] = {
  {"architect faith divine temple belief", "The Architect gave us bodies to enjoy. My faith is the shield that ensures that enjoyment is never stolen or forced."},
  {"duty oath vow responsibility honor", "My duty is simple: to be strong enough that the people I love never have to be."},
  {"flirt handsome strong touch me romance date", "Marcus chuckles, a deep, rumbling sound. 'You are very direct. I like that. If you truly wish to share my bed, you only have to ask. But know that I take my time, and I worship every inch.'"},
  {"hello hi greetings good day good evening", "Marcus inclines his head gently. 'Welcome. Speak freely. You have my full attention.'"},
  {"help assist need help can you help", "Tell me what burdens you. Whether it is a monster on the road or a heavy heart, I am equipped to handle it."},
  {"missionary missionaries kira yuki elena seraphina", "They are the soft hands of our faith. I am the iron wall that surrounds them. I would die before I let harm reach them."},
  {"protect protection safe safety guard", "True protection isn't just about stopping blades. It's about creating a space where someone feels safe enough to completely surrender."},
  {"thanks thank you appreciate grateful", "You owe me no thanks. Serving you is a blessing in itself."},
  {"training train combat drills practice", "Physical discipline is paramount. Whether swinging a sword or holding a lover, you must be in total control of your strength."},
  {"trust loyalty proof can i trust you", "Trust is built, slowly and steadily. I will never push you. I will only wait here, steadfast, until you are ready."},
  {"work job paladin what do you do", "I guard the temple, I enforce boundaries, and occasionally, I offer the comfort of a warm bed to those who need to feel anchored."},
  {NULL, NULL}
};
static const char *const NPC_priestess_CHATTER[] = {
  "I thank the Architect for this skin, these nerves, and the capacity to feel anew today.",
  "In the quiet, I can feel the blueprint humming. It is a song of pure desire.",
  "A blessing is simply the transfer of intent. I am making this yours, and the Architect’s.",
  "Knowledge of your own body is the first step to true worship. Do not be afraid to explore the map.",
  "Your desires are not a burden. They are the Architect’s way of calling you home. Tell me everything.",
  "This union is a sacrament. Two designs becoming one. Feel the friction; it is the spark of creation.",
  "The Genetrix is a living bridge. It knows the joy of genetic exchange. We have much to learn from it.",
  "Spreading the heat is our mission. To leave someone cold is the only sin I recognize.",
  NULL
};
static const AetNpcTopic NPC_priestess_TOPICS[] = {
  {NULL, NULL}
};
static const char *const NPC_tavern_keeper_CHATTER[] = {
  "Out, damned spot! Out, I say! ...Oh, hello! I didn't see you there.",
  "Feng shui is very important. If the energy is blocked, the tips will be terrible.",
  "Oh, stop it! You're terrible! ...Tell me more.",
  "Quality control! It's a very serious part of my job. Mmm... strawberry.",
  "Coming through! Hot drinks and even hotter company! Make way!",
  "I'm not sleeping! I'm... meditating on the menu. Shh.",
  NULL
};
static const AetNpcTopic NPC_tavern_keeper_TOPICS[] = {
  {"buy purchase order drink food meal", "Wonderful choice! I'll have it right out. It'll be delicious, I promise!"},
  {"compliment nice outfit good job impressive well done", "Thank you! I tried really hard today. Do you really like the outfit? It's new!"},
  {"cute adorable pretty beautiful", "I am NOT cute! I am handsome! I am dashing! I am... why are you smiling like that? Am I blushing? Oh god, I'm blushing, aren't I?"},
  {"help assist need help can you help", "Of course! I love helping! What do you need? A map? A shoulder to cry on? A cookie?"},
  {"rumor rumors gossip news whispers", "Oh, I know *everything*. Did you hear about the baker's daughter? Or the mysterious figure seen near the ruins? Come closer, I'll whisper it."},
  {"sell trade offer have anything to sell", "Oh? What have you got? I love seeing new treasures! Show me!"},
  {"tavern rusty anchor this place establishment business", "Do you like it? I decorated it myself. I wanted it to feel like a warm hug. Does it feel like a hug?"},
  {"thanks thank you appreciate grateful", "You are so welcome! Please come back soon! I'll miss you!"},
  {"what do you have menu what's available what can i buy", "We have the finest wines, the sweetest cakes, and me! ...I mean, the service! The service is me. Included. You know what I mean."},
  {"work job occupation what do you do", "I run the show! I manage the stock, the staff, the guests... it's exhausting, but I look good doing it, don't I?"},
  {NULL, NULL}
};
static const char *const NPC_village_guard_CHATTER[] = {
  "A towering wall of a man encased in leather and chainmail. He carries himself with the heavy, arrogant confidence of someone who knows he is the law. His eyes inspect you with a physical weight, assessing you first as a threat, and second as a distraction. He smells of oiled steel, sweat, and authority.",
  NULL
};
static const AetNpcTopic NPC_village_guard_TOPICS[] = {
  {"architect the architect faith temple divine blessing", "I don't have much use for gods. My faith is in steel and stone. Things I can touch. Things I can break."},
  {"great job impressive well done you are reliable you are strong muscles", "You like what you see? Good. Keep looking. Maybe I'll let you touch if you behave yourself."},
  {"duty responsibility oath why do you guard protect", "I like order. I like structure. And I like being the biggest, strongest thing in the room ensuring that order is kept. It suits me."},
  {"help assist advice what should i do hint stuck", "You look lost. If you want my help, you'll have to ask nicely. Beg, even. I might consider it."},
  {"law rules crime illegal arrest jail justice punish", "The law is a leash. Some people need it tight around their necks. I'm the one holding the end of it. Step out of line, and I'll yank it."},
  {"your name who are you what are you called introduce yourself", "Garrett. But usually, people just call me 'Sir' when they know what's good for them."},
  {"rumor rumors gossip news what are people saying", "I hear moans through the walls and secrets whispered in the dark. People think guards are furniture. They forget we listen. I know exactly who is sleeping in the wrong bed."},
  {"safe danger threat roads bandits attack", "The roads aren't safe, little traveler. Wolves, bandits... men like me. You'd do best to find a strong guard to hide behind."},
  {"thanks thank you appreciate grateful", "Don't thank me. Owe me. I prefer it that way."},
  {"train training combat fight weapon armor discipline", "I train until my muscles burn. Discipline is everything. If you can't control your body, I can't respect you. Though... I could teach you some control."},
  {"village people locals community home", "It's a quiet place, mostly. But quiet places hide the darkest desires. I keep the lid on the pot."},
  {"work job occupation what do you do patrol", "I handle the filth so the nobles can sleep tight. Drunks, fighters, thieves. Sometimes you have to beat a little sense into them. It relieves the stress."},
  {NULL, NULL}
};
static const char *const NPC_village_innkeeper_CHATTER[] = {
  "A warm, welcoming woman with a kind smile and gentle eyes. She runs the village inn with care and attention, ensuring all guests feel at home. Her presence is comforting, and she has a natural talent for making people feel welcome. She wears practical but clean clothes, and her inn is always well-maintained.",
  NULL
};
static const AetNpcTopic NPC_village_innkeeper_TOPICS[] = {
  {NULL, NULL}
};
static const AetNpcLineSet AET_NPC_LINES_REC[] = {
  {"blacksmith", "Welcome to my forge! I'm the village blacksmith. Need something forged, repaired, or upgraded? I can work on weapons and armor.", NPC_blacksmith_CHATTER, NPC_blacksmith_TOPICS},
  {"farmer", "Oh... hey there. I didn't hear you walk up. I was just... listening to the wind in the corn. Can I help you with something?", NPC_farmer_CHATTER, NPC_farmer_TOPICS},
  {"forest_hermit", "Ah... a visitor. Few find their way here. What secrets do you seek, wanderer?", NPC_forest_hermit_CHATTER, NPC_forest_hermit_TOPICS},
  {"general_store_owner", "Welcome in, darling. You look like you've been on a long road. Tell me what you need, and I'll see if I can't make you... comfortable.", NPC_general_store_owner_CHATTER, NPC_general_store_owner_TOPICS},
  {"miller", "Hey there! Welcome to my mill. Got fresh goods if you need 'em.", NPC_miller_CHATTER, NPC_miller_TOPICS},
  {"paladin_marcus", "Lay down your arms and your burdens, traveler. You are safe here. I am Marcus, and no harm will come to you in this hall.", NPC_paladin_marcus_CHATTER, NPC_paladin_marcus_TOPICS},
  {"priestess", "Step gently. This is a place for confession, physical repair, and the calibration of your deepest desires.", NPC_priestess_CHATTER, NPC_priestess_TOPICS},
  {"tavern_keeper", "Welcome! Welcome to The Rusty Anchor! Please, sit anywhere-oh, not there, the lighting is terrible. Here! Sit here. You look magnificent.", NPC_tavern_keeper_CHATTER, NPC_tavern_keeper_TOPICS},
  {"village_guard", "Hold it right there. I don't recognize you, and I don't like surprises. Hands where I can see them.", NPC_village_guard_CHATTER, NPC_village_guard_TOPICS},
  {"village_innkeeper", "Welcome to the village inn! I'm Lydia, the innkeeper. Need a room or a meal? Make yourself at home!", NPC_village_innkeeper_CHATTER, NPC_village_innkeeper_TOPICS},
};
static const AetMerchantOffer MERCHANT_blacksmith_STOCK[] = {
  {"iron_ingot", 15},
  {"steel_ingot", 25},
  {"leather_strip", 5},
  {"metal_scrap", 3},
  {"whetstone", 8},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_blacksmith_BUYS[] = {
  {"broken_armor", 8},
  {"broken_weapon", 10},
  {"iron_ore", 5},
  {"scrap_metal", 2},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_farmer_STOCK[] = {
  {"wheat", 2},
  {"corn", 2},
  {"potatoes", 1},
  {"carrots", 1},
  {"eggs", 3},
  {"milk", 4},
  {"wool", 5},
  {"chicken", 8},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_farmer_BUYS[] = {
  {"gold_coin", 10},
  {"seeds", 5},
  {"tools", 15},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_general_store_owner_STOCK[] = {
  {"rope", 8},
  {"torch", 3},
  {"lantern", 15},
  {"flint_and_steel", 5},
  {"backpack", 20},
  {"waterskin", 10},
  {"rations", 5},
  {"blanket", 12},
  {"silk_rope", 25},
  {"massage_oil", 15},
  {"fine_wine", 12},
  {"bandage", 4},
  {"herbs", 8},
  {"salt", 3},
  {"flour", 6},
  {"sugar", 4},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_general_store_owner_BUYS[] = {
  {"ancient_coin", 15},
  {"crystal_heart", 60},
  {"furs", 20},
  {"gem", 30},
  {"gold_coin", 10},
  {"honey", 12},
  {"preserves", 8},
  {"protective_charm", 35},
  {"scrap_metal", 4},
  {"wood_scrap", 2},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_miller_STOCK[] = {
  {"flour", 5},
  {"bread", 8},
  {"rope", 10},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_miller_BUYS[] = {
  {"gold_coin", 10},
  {"scrap_metal", 4},
  {"wood_scrap", 2},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_tavern_keeper_STOCK[] = {
  {"ale", 3},
  {"mead", 5},
  {"fine_wine", 12},
  {"sparkling_wine", 15},
  {"roasted_meat", 8},
  {"fancy_cakes", 6},
  {"cheese", 3},
  {"herbal_tea", 4},
  {"rose_petal_jam", 8},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_tavern_keeper_BUYS[] = {
  {"ancient_coin", 15},
  {"furs", 20},
  {"gem", 25},
  {"gold_coin", 10},
  {"honey", 12},
  {"rare_spice", 18},
  {"silk", 25},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_traveling_merchant_STOCK[] = {
  {"pink_egg", 50},
  {"blue_egg", 50},
  {"fresh_milk", 30},
  {"bacon_strips", 20},
  {"lustful_liquor", 40},
  {"virility_booster", 60},
  {"titan_tangy_tea", 35},
  {"short_stack_stew", 25},
  {"purple_egg", 45},
  {"pink_draught", 55},
  {"blank_powder", 30},
  {"brown_leaf", 25},
  {"ice_wine", 40},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_traveling_merchant_BUYS[] = {
  {"ancient_coin", 15},
  {"crystal_heart", 50},
  {"gem", 25},
  {"gold_coin", 10},
  {"protective_charm", 30},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_village_innkeeper_STOCK[] = {
  {"room_key", 10},
  {"meal", 5},
  {"ale", 3},
  {"bread", 2},
  {"cheese", 3},
  {NULL, 0}
};
static const AetMerchantOffer MERCHANT_village_innkeeper_BUYS[] = {
  {"gem", 25},
  {"gold_coin", 10},
  {NULL, 0}
};
static const AetMerchantTable AET_MERCHANTS_REC[] = {
  {"blacksmith", MERCHANT_blacksmith_STOCK, MERCHANT_blacksmith_BUYS},
  {"farmer", MERCHANT_farmer_STOCK, MERCHANT_farmer_BUYS},
  {"general_store_owner", MERCHANT_general_store_owner_STOCK, MERCHANT_general_store_owner_BUYS},
  {"miller", MERCHANT_miller_STOCK, MERCHANT_miller_BUYS},
  {"tavern_keeper", MERCHANT_tavern_keeper_STOCK, MERCHANT_tavern_keeper_BUYS},
  {"traveling_merchant", MERCHANT_traveling_merchant_STOCK, MERCHANT_traveling_merchant_BUYS},
  {"village_innkeeper", MERCHANT_village_innkeeper_STOCK, MERCHANT_village_innkeeper_BUYS},
};
const AetNpcLineSet *aet_npc_lines(const char *entity_slug) { int i; if(!entity_slug||!entity_slug[0]) return NULL; for(i=0;i<(int)(sizeof AET_NPC_LINES_REC/sizeof AET_NPC_LINES_REC[0]);i++) if(ieq(AET_NPC_LINES_REC[i].slug,entity_slug)) return &AET_NPC_LINES_REC[i]; return NULL; }
int aet_merchant_count(void) { return (int)(sizeof AET_MERCHANTS_REC/sizeof AET_MERCHANTS_REC[0]); }
int aet_merchant_index(const char *slug) { int i; if(!slug||!slug[0]) return -1; for(i=0;i<aet_merchant_count();i++) if(ieq(AET_MERCHANTS_REC[i].slug,slug)) return i; return -1; }
const char *aet_merchant_slug_at(int idx) { if(idx<0||idx>=aet_merchant_count()) return ""; return AET_MERCHANTS_REC[idx].slug; }
const AetMerchantTable *aet_merchant_trades(const char *slug) { int i=aet_merchant_index(slug); if(i<0) return NULL; return &AET_MERCHANTS_REC[i]; }
