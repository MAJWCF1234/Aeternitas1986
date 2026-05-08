window.WORLD_MAP_DATA = {
  "generatedAt": "2026-05-07T22:07:15.815Z",
  "roomCount": 161,
  "npcCount": 19,
  "rooms": [
    {
      "id": "west_of_house",
      "name": "West of House",
      "description": "You are standing in an open field west of a white house, with a boarded front door. The grass is overgrown and wildflowers dot the landscape, swaying gently in the breeze. A small mailbox stands sentinel near the path, its red flag rusted in the down position. The house looms before you, its windows dark and foreboding like empty eye sockets. The paint has peeled away in patches, revealing weathered wood beneath. A faint breeze rustles the nearby trees, carrying with it the scent of damp earth and something else—something ancient and forgotten. The air feels heavy, as if the very atmosphere is holding its breath.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "mailbox",
        "scrap_metal",
        "wood_scrap",
        "lockpick",
        "rusty_pick"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "traveling_merchant",
          "name": "Corbin"
        }
      ],
      "exitCount": 4,
      "exits": {
        "north": "north_of_house",
        "south": "south_of_house",
        "east": "front_door",
        "down": "hidden_cellar"
      }
    },
    {
      "id": "north_of_house",
      "name": "North of House",
      "description": "You are facing the north side of a white house. There is no door here, just weathered wooden walls that have seen better days. A window is visible but appears sealed shut with thick boards. Ivy creeps up the walls, giving the place an ancient, forgotten feel. A narrow path leads north into the forest.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "south": "west_of_house",
        "east": "east_of_house",
        "west": "west_of_house",
        "north": "forest_path"
      }
    },
    {
      "id": "south_of_house",
      "name": "South of House",
      "description": "You are behind the house. A small garden plot lies fallow, choked with weeds and dead plants. A rusty gate leads to what might be a cellar entrance. The air here is thick and musty, and you hear the faint sound of dripping water.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "gate"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "north": "west_of_house",
        "east": "east_of_house",
        "south": "garden",
        "down": "cellar"
      }
    },
    {
      "id": "east_of_house",
      "name": "East of House",
      "description": "You are on the east side of the house. A well-maintained path leads to a small shed that looks surprisingly intact compared to the house. You can see the front door from here, and notice strange symbols carved into the door frame.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "shed_door"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "west": "west_of_house",
        "north": "north_of_house",
        "south": "south_of_house",
        "east": "shed"
      }
    },
    {
      "id": "front_door",
      "name": "Front Door",
      "description": "You are at the front door of the white house. The door is boarded up with heavy planks, but you notice a small keyhole that seems to glow faintly. Strange runes are carved into the door frame, and you feel a chill as you approach.",
      "locked": true,
      "region": "Hollow Ridge",
      "items": [
        "door"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "west_of_house",
        "east": "inside_house"
      }
    },
    {
      "id": "inside_house",
      "name": "Inside House - Foyer",
      "description": "You are in the foyer of the mysterious white house. Dust motes dance in shafts of light that filter through the gaps in the boarded windows, creating ethereal beams that illuminate floating particles. The air is thick with the scent of old wood, dust, and something else—something that speaks of forgotten memories and long-lost lives. A grand staircase sweeps upward, its banister carved with intricate patterns now obscured by layers of grime. The steps creak ominously as if warning you of what lies above. Doorways branch off in multiple directions, each leading deeper into the house's mysteries. The floorboards groan underfoot, their ancient wood protesting every step. Portraits line the walls, their subjects' eyes seeming to follow you as you move.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "table",
        "book",
        "lantern",
        "coat_rack",
        "mirror"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 6,
      "exits": {
        "east": "front_door",
        "up": "attic",
        "down": "basement",
        "north": "living_room",
        "south": "dining_room",
        "west": "kitchen"
      }
    },
    {
      "id": "living_room",
      "name": "Living Room",
      "description": "A spacious living room with faded furniture arranged around a cold, dark fireplace. The mantelpiece is carved with intricate designs, now obscured by layers of dust and cobwebs. Tattered curtains hang limply from the windows, their once-vibrant colors faded to muted shades of gray and beige. An old Persian rug covers most of the floor, its patterns worn smooth in places where countless feet have trod. Bookshelves line one wall, filled with dusty volumes whose spines have cracked with age. A comfortable armchair sits near the fireplace, its upholstery torn and stuffing spilling out in places. A coffee table holds an empty vase, its surface marred by water rings and scratches. The air here feels still and heavy, as if time itself has slowed to a crawl.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "armchair",
        "coffee_table",
        "vase",
        "bookshelf",
        "fireplace",
        "old_rug",
        "curtains"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "south": "inside_house",
        "east": "study",
        "west": "bedroom_hallway"
      }
    },
    {
      "id": "study",
      "name": "Study",
      "description": "A cozy study filled with books and papers. A large desk sits beneath a window, covered in scattered documents and an old inkwell. Bookshelves reach to the ceiling, filled with leather-bound volumes. A globe sits in one corner, and a comfortable reading chair faces the desk. The air smells of old paper and ink.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "desk",
        "inkwell",
        "quill",
        "papers",
        "globe",
        "reading_chair",
        "old_books",
        "engineering_tome",
        "merchant_ledger",
        "fine_lockpick"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "living_room",
        "north": "library"
      }
    },
    {
      "id": "library",
      "name": "Library",
      "description": "A small private library with floor-to-ceiling bookshelves. Ladders lean against the shelves, and reading nooks are scattered throughout. The room is quiet except for the sound of pages rustling in the breeze from a cracked window. A small table holds a reading lamp and several open books.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
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
        "herbalism_guide"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "study"
      }
    },
    {
      "id": "dining_room",
      "name": "Dining Room",
      "description": "A formal dining room with a long wooden table that could seat twelve. Dusty place settings remain on the table, as if waiting for guests who never arrived. A chandelier hangs overhead, its crystals dulled with age. A sideboard holds tarnished silverware and empty serving dishes. Portraits of stern-looking ancestors line the walls.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "dining_table",
        "chairs",
        "place_settings",
        "chandelier",
        "sideboard",
        "silverware",
        "portraits"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "inside_house",
        "east": "kitchen"
      }
    },
    {
      "id": "kitchen",
      "name": "Kitchen",
      "description": "A large kitchen with an old cast-iron stove and a stone sink. Cupboards line the walls, some hanging open to reveal empty shelves. A butcher block table sits in the center, and herbs hang drying from the ceiling. The room smells faintly of old spices and wood smoke. A door leads to a pantry, and another to the backyard.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stove",
        "sink",
        "cupboards",
        "butcher_block",
        "drying_herbs",
        "pots",
        "pans"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "east": "inside_house",
        "west": "dining_room",
        "north": "pantry",
        "south": "backyard"
      }
    },
    {
      "id": "pantry",
      "name": "Pantry",
      "description": "A small pantry filled with empty shelves and storage containers. Jars line the walls, most empty but a few still containing preserved foods. The air is cool and smells of earth and old preserves. A small window lets in dim light.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "empty_jars",
        "preserved_foods",
        "storage_containers",
        "shelves"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "kitchen"
      }
    },
    {
      "id": "bedroom_hallway",
      "name": "Bedroom Hallway",
      "description": "A narrow hallway leading to the bedrooms. Doors line both sides, and a window at the end looks out over the garden. The hallway is quiet, with only the sound of your footsteps on the wooden floor.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hallway_window",
        "door_frames"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "east": "living_room",
        "north": "master_bedroom",
        "south": "guest_bedroom",
        "west": "bathroom"
      }
    },
    {
      "id": "master_bedroom",
      "name": "Master Bedroom",
      "description": "A large bedroom with a four-poster bed draped in faded curtains. A wardrobe stands against one wall, and a vanity table sits beneath a window. The room feels lived-in but abandoned, with personal items still scattered about. A door leads to a private bathroom.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "four_poster_bed",
        "bed_curtains",
        "wardrobe",
        "vanity_table",
        "personal_items",
        "dresser"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "south": "bedroom_hallway",
        "east": "master_bathroom"
      }
    },
    {
      "id": "master_bathroom",
      "name": "Master Bathroom",
      "description": "A private bathroom attached to the master bedroom. An old clawfoot bathtub sits in one corner, and a mirror hangs above a sink. The room is clean but shows signs of age, with cracked tiles and tarnished fixtures.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bathtub",
        "sink",
        "mirror",
        "towel_rack",
        "medicine_cabinet"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "master_bedroom"
      }
    },
    {
      "id": "guest_bedroom",
      "name": "Guest Bedroom",
      "description": "A smaller bedroom, clearly meant for guests. A simple bed sits against one wall, and a small desk and chair occupy a corner. The room is sparsely furnished but comfortable. A window looks out over the garden.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "desk",
        "chair",
        "window",
        "wardrobe"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "bedroom_hallway"
      }
    },
    {
      "id": "bathroom",
      "name": "Bathroom",
      "description": "A shared bathroom with basic amenities. A sink, toilet, and shower stall occupy the small space. The room is functional but shows its age, with worn fixtures and faded tiles.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "sink",
        "toilet",
        "shower",
        "towel_rack",
        "medicine_cabinet"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "bedroom_hallway"
      }
    },
    {
      "id": "backyard",
      "name": "Backyard",
      "description": "A spacious backyard behind the house. Overgrown grass and wildflowers cover the area, and a stone path leads to a small shed. An old swing hangs from a tree branch, and a vegetable garden plot lies fallow. The area feels peaceful but neglected.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "swing",
        "vegetable_garden",
        "stone_path",
        "wildflowers"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "north": "kitchen",
        "east": "garden",
        "west": "backyard_shed"
      }
    },
    {
      "id": "backyard_shed",
      "name": "Backyard Shed",
      "description": "A small wooden shed filled with gardening tools and supplies. Rakes, shovels, and other implements hang from the walls. The air smells of earth and old wood. A small window lets in dim light.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rake",
        "shovel",
        "gardening_tools",
        "seeds",
        "pots"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "backyard"
      }
    },
    {
      "id": "attic",
      "name": "Attic",
      "description": "The attic is filled with old furniture covered in dust sheets. A small chest sits in the corner, and cobwebs hang from the rafters.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "chest",
        "dust_sheet"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "inside_house"
      }
    },
    {
      "id": "basement",
      "name": "Basement",
      "description": "The basement is dark and damp. Water drips from somewhere in the darkness, and the air is heavy with the smell of earth and decay.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "old_box"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "inside_house"
      }
    },
    {
      "id": "garden",
      "name": "Garden",
      "description": "A peaceful garden with overgrown paths and wildflowers. A small fountain bubbles quietly in the center. Beyond the fountain, an old stone well sits nestled among wild roses.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fountain",
        "herbs",
        "house_key"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "south_of_house",
        "east": "old_well"
      }
    },
    {
      "id": "forest_path",
      "name": "Forest Path",
      "description": "You stand on a narrow, winding path through dense forest. Ancient trees tower overhead, their branches forming a natural canopy. The air is cool and smells of damp earth and pine. Strange sounds echo from deeper in the woods. A rough stone marker stands beside the path, carved with cryptic symbols. To the northeast, you can see a well-traveled road leading toward what appears to be a village.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stone_marker",
        "fallen_branch"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "wolf",
          "name": "Wolf"
        }
      ],
      "exitCount": 6,
      "exits": {
        "south": "north_of_house",
        "north": "forest_clearing",
        "east": "swamp_clearing",
        "northeast": "village_road",
        "northwest": "deep_forest",
        "west": "meadow"
      }
    },
    {
      "id": "deep_forest",
      "name": "Deep Forest",
      "description": "You venture deeper into the ancient forest. The trees here are massive, their trunks so wide you could hide behind them. Sunlight barely penetrates the thick canopy above, creating a twilight atmosphere. The forest floor is covered in moss and fallen leaves. Strange, glowing mushrooms dot the ground, and you hear the distant call of unknown creatures. In a small clearing, you can see a simple hut - the home of the forest hermit.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "glowing_mushrooms",
        "moss",
        "fallen_leaves",
        "ancient_tree_bark"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "forest_hermit",
          "name": "Thorn"
        }
      ],
      "exitCount": 4,
      "exits": {
        "southeast": "forest_path",
        "east": "forest_clearing",
        "north": "ancient_grove",
        "enter": "hermit_hut"
      }
    },
    {
      "id": "hermit_hut",
      "name": "Hermit's Hut",
      "description": "A simple, weathered hut hidden deep in the forest. The structure is old but well-maintained, blending seamlessly with the natural surroundings. Inside, the hut is filled with ancient tomes, strange artifacts, and the scent of herbs and incense. The hermit's presence fills the space with an aura of ancient wisdom.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_tome",
        "herb_bundle",
        "mysterious_artifact"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "exit": "deep_forest"
      }
    },
    {
      "id": "ancient_grove",
      "name": "Ancient Grove",
      "description": "A sacred grove where the oldest trees stand in a circle. The air here feels different - charged with ancient energy. A stone altar sits in the center, covered in moss and strange symbols. The grove is quiet, almost reverent, as if holding its breath. You feel like you're standing in a place of great significance. A narrow, almost invisible path leads north into deeper shadows.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stone_altar",
        "ancient_symbols",
        "moss",
        "standing_stones"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "south": "deep_forest",
        "enter": "nexus_point_1",
        "north": "hidden_shrine"
      }
    },
    {
      "id": "meadow",
      "name": "Wildflower Meadow",
      "description": "A beautiful meadow filled with wildflowers of every color. Butterflies dance among the blooms, and bees buzz lazily from flower to flower. The grass is tall and soft, and a gentle breeze makes the flowers sway. In the distance, you can see a small pond reflecting the sky. The meadow feels peaceful and alive.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "wildflowers",
        "butterflies",
        "tall_grass"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "east": "forest_path",
        "north": "pond",
        "south": "hilltop"
      }
    },
    {
      "id": "pond",
      "name": "Tranquil Pond",
      "description": "A small, clear pond surrounded by reeds and water lilies. The water is still and reflects the sky perfectly. Dragonflies dart across the surface, and you can see small fish swimming in the depths. A wooden dock extends into the water, though it looks weathered. The area is peaceful and serene.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "water_lilies",
        "reeds",
        "wooden_dock",
        "fishing_spot"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "south": "meadow",
        "east": "stream"
      }
    },
    {
      "id": "stream",
      "name": "Babbling Stream",
      "description": "A small stream that flows through the forest, its clear water bubbling over smooth stones. The sound of running water is soothing. Small fish dart in the shallows, and you can see where animals have come to drink. The stream flows from the north and disappears into the forest to the south.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "smooth_stones",
        "stream_water",
        "fish"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "west": "pond",
        "north": "stream_source",
        "south": "forest_path"
      }
    },
    {
      "id": "stream_source",
      "name": "Stream Source",
      "description": "The source of the stream - a natural spring bubbling up from the ground. The water is crystal clear and cold. Ferns and moss grow around the spring, creating a lush, green area. The water flows down a small waterfall into the stream below. This feels like a place of natural power.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "natural_spring",
        "waterfall",
        "ferns",
        "moss"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "stream"
      }
    },
    {
      "id": "hilltop",
      "name": "Hilltop Overlook",
      "description": "A high hilltop that offers a stunning view of the surrounding countryside. From here, you can see the white house in the distance, the forest stretching out below, and the village beyond. The wind is stronger here, and wild grasses sway in the breeze. A large boulder provides a place to sit and take in the view.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "boulder",
        "wild_grasses",
        "viewpoint"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "north": "meadow",
        "east": "rocky_outcrop",
        "down": "hillside_path"
      }
    },
    {
      "id": "rocky_outcrop",
      "name": "Rocky Outcrop",
      "description": "A rocky area where large boulders have tumbled down from the hills above. The rocks are covered in lichen and moss, and small plants grow in the crevices. The area feels ancient and weathered. You can see where animals have made paths between the rocks.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "boulders",
        "lichen",
        "moss",
        "rock_crevices"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "hilltop",
        "south": "cave_entrance"
      }
    },
    {
      "id": "cave_entrance",
      "name": "Cave Entrance",
      "description": "The entrance to a small cave in the hillside. The opening is partially hidden by overgrown vegetation. Inside, you can see the cave extends into darkness. The air coming from the cave is cool and damp. Strange sounds echo from within.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "overgrown_vegetation",
        "cave_opening"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "rocky_outcrop",
        "inside": "cave_interior"
      }
    },
    {
      "id": "cave_interior",
      "name": "Cave Interior",
      "description": "The interior of the cave is dark and cool. Stalactites hang from the ceiling, and the floor is uneven with natural stone formations. A small pool of water collects in one corner, and strange crystals grow from the walls, glowing faintly. The cave extends deeper into the hillside.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stalactites",
        "water_pool",
        "glowing_crystals",
        "stone_formations"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "out": "cave_entrance",
        "deeper": "cave_deep"
      }
    },
    {
      "id": "cave_deep",
      "name": "Deep Cave",
      "description": "The deepest part of the cave. The air is still and heavy here. The glowing crystals are more numerous, casting an ethereal light. Ancient markings cover the walls, and you can feel a sense of mystery and power. Something important might be hidden here.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_markings",
        "more_crystals",
        "hidden_chamber"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "out": "cave_interior"
      }
    },
    {
      "id": "hillside_path",
      "name": "Hillside Path",
      "description": "A winding path that leads down the hillside. The path is well-worn, suggesting regular use. Wildflowers grow along the edges, and you can see where small animals have crossed. The path leads back toward the house and forest.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "path_markers",
        "wildflowers"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "up": "hilltop",
        "down": "forest_path"
      }
    },
    {
      "id": "village_road",
      "name": "Village Road",
      "description": "A well-maintained dirt road winds through the forest toward a small village. The path is wide enough for carts, and you can see wagon ruts in the mud. Smoke rises from chimneys in the distance, and the sound of voices and activity drifts on the breeze. A wooden signpost points toward 'The Rusty Anchor Tavern'.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "signpost",
        "roadside_stone"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "southwest": "forest_path",
        "north": "tavern_exterior",
        "east": "village_square"
      }
    },
    {
      "id": "tavern_exterior",
      "name": "Outside The Rusty Anchor",
      "description": "You stand before a two-story wooden building with a weathered sign that reads 'The Rusty Anchor Tavern'. The structure looks well-maintained despite its age, with warm light spilling from the windows. A hitching post stands to one side, and a small stable is visible around back. The smell of ale, roasted meat, and wood smoke fills the air. The front door is open, inviting you inside.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "tavern_sign",
        "hitching_post",
        "lantern"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "south": "village_road",
        "north": "tavern_common_room",
        "east": "tavern_stables",
        "west": "tavern_back_alley"
      }
    },
    {
      "id": "tavern_common_room",
      "name": "The Rusty Anchor - Common Room",
      "description": "The main room of the tavern is warm and welcoming, filled with the sounds of conversation, laughter, and clinking mugs. A long wooden bar dominates one wall, behind which shelves are stocked with bottles and casks. Several round tables are scattered throughout, some occupied by patrons. A large fireplace crackles merrily, casting dancing shadows. Stairs lead up to the rooms above, and you can see doorways leading to the kitchen and a back room.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
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
        "cheese"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "tavern_keeper",
          "name": "Soren"
        },
        {
          "id": "bartender",
          "name": "Silas (The Bartender)"
        }
      ],
      "exitCount": 5,
      "exits": {
        "south": "tavern_exterior",
        "north": "tavern_kitchen",
        "east": "tavern_back_room",
        "up": "tavern_stairs",
        "west": "tavern_privy"
      }
    },
    {
      "id": "tavern_kitchen",
      "name": "Tavern Kitchen",
      "description": "A bustling kitchen filled with the aromas of cooking. A large hearth with a spit dominates one wall, currently roasting what smells like pork. Pots and pans hang from hooks, and a large wooden table serves as a prep area. Barrels of flour, salt, and other supplies line the walls. A door leads to what appears to be a storage cellar.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hearth",
        "cooking_pot",
        "spit",
        "prep_table",
        "flour_barrel",
        "salt_barrel",
        "stew"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "south": "tavern_common_room",
        "down": "tavern_cellar"
      }
    },
    {
      "id": "tavern_cellar",
      "name": "Tavern Cellar",
      "description": "A cool, dark cellar beneath the tavern. Barrels of ale and wine line the walls, and shelves hold preserved foods, pickled vegetables, and dried meats. The air is musty with the scent of aging alcohol and earth. A ladder leads back up to the kitchen.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ale_barrel",
        "wine_barrel",
        "preserved_foods",
        "pickled_vegetables",
        "dried_meat"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "tavern_kitchen"
      }
    },
    {
      "id": "tavern_back_room",
      "name": "Tavern Back Room",
      "description": "A smaller, more private room separated from the main common area. This space is quieter, with a few tables and comfortable chairs. The walls are decorated with old maps and nautical artifacts. A door in the corner leads to what might be a private meeting space. This room is often used for more intimate conversations or business dealings.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "comfortable_chairs",
        "old_maps",
        "nautical_artifacts",
        "candles"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "tavern_common_room",
        "north": "tavern_private_room"
      }
    },
    {
      "id": "tavern_private_room",
      "name": "Private Meeting Room",
      "description": "A secluded room with thick walls and a heavy door. This space is designed for privacy, with a single table and chairs. The room is dimly lit by a single lantern. This is where sensitive conversations happen, away from prying ears.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "private_table",
        "chairs",
        "lantern",
        "locked_chest"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "tavern_back_room"
      }
    },
    {
      "id": "tavern_stairs",
      "name": "Tavern Stairway",
      "description": "A narrow wooden staircase leads up to the second floor. The steps creak slightly underfoot, and the walls are lined with old portraits and tapestries. You can hear muffled sounds from the rooms above.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "portraits",
        "tapestries"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "down": "tavern_common_room",
        "up": "tavern_hallway"
      }
    },
    {
      "id": "tavern_hallway",
      "name": "Tavern Upper Hallway",
      "description": "A long hallway on the second floor with several doors leading to guest rooms. A window at the end of the hall looks out over the village. The floorboards are well-worn from years of use. Each door has a small number plate.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hallway_window",
        "number_plates"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 5,
      "exits": {
        "down": "tavern_stairs",
        "north": "tavern_room_1",
        "east": "tavern_room_2",
        "south": "tavern_room_3",
        "west": "tavern_room_4"
      }
    },
    {
      "id": "tavern_room_1",
      "name": "Guest Room 1",
      "description": "A simple but comfortable guest room. A bed with a straw mattress sits against one wall, and a small table and chair occupy the corner. A window looks out over the village. The room is clean and well-maintained, if spartan.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "straw_mattress",
        "table",
        "chair",
        "window"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "tavern_hallway"
      }
    },
    {
      "id": "tavern_room_2",
      "name": "Guest Room 2",
      "description": "Another guest room, similar to the others but with a slightly larger bed. A washbasin sits on a stand, and a small chest at the foot of the bed could hold belongings. The room has a cozy, lived-in feel.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "washbasin",
        "chest",
        "window"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "tavern_hallway"
      }
    },
    {
      "id": "tavern_room_3",
      "name": "Guest Room 3",
      "description": "A guest room with a view of the stable yard. This room is slightly larger and includes a small writing desk. The bed looks more comfortable than the others, with a thicker mattress.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "comfortable_mattress",
        "writing_desk",
        "window"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "tavern_hallway"
      }
    },
    {
      "id": "tavern_room_4",
      "name": "Guest Room 4",
      "description": "The last guest room in the hallway. This one has a small balcony overlooking the back alley. The room is well-furnished with a bed, a wardrobe, and a small table. It's the most private of the guest rooms.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "wardrobe",
        "table",
        "balcony"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "tavern_hallway"
      }
    },
    {
      "id": "tavern_stables",
      "name": "Tavern Stables",
      "description": "A small stable behind the tavern where travelers can board their horses. The structure is simple but sturdy, with several stalls. Hay is stacked in one corner, and a water trough sits in the center. The smell of hay, horses, and leather fills the air.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "horse_stalls",
        "hay",
        "water_trough",
        "tack"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "tavern_exterior",
        "south": "tavern_back_alley"
      }
    },
    {
      "id": "tavern_back_alley",
      "name": "Tavern Back Alley",
      "description": "A narrow alley behind the tavern, used for deliveries and waste disposal. Barrels and crates are stacked against the walls. A door leads into the kitchen, and you can see the stable entrance further down. The alley is dimly lit and somewhat secluded.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "barrels",
        "crates",
        "delivery_door"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "north": "tavern_stables",
        "east": "tavern_exterior",
        "west": "tavern_kitchen"
      }
    },
    {
      "id": "tavern_privy",
      "name": "Tavern Privy",
      "description": "A small outbuilding behind the tavern serving as the privy. It's a simple structure with basic facilities. The door can be latched from the inside for privacy.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "privy_seat",
        "latch"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "tavern_common_room"
      }
    },
    {
      "id": "village_square",
      "name": "Village Square",
      "description": "The central square of a small village. A well sits in the center, and several shops and buildings surround the open space. People go about their daily business, and the atmosphere is lively but peaceful. The Rusty Anchor Tavern is visible to the west. In the center of the square stands a monolithic structure - a Nexus Point that connects to other locations across the realm. A well-maintained path leads north toward the kingdom of Hollow Ridge. The village guard patrols the area, keeping watch.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "well",
        "market_stall"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "traveling_bard",
          "name": "Aria"
        },
        {
          "id": "village_guard",
          "name": "Garrett"
        }
      ],
      "exitCount": 8,
      "exits": {
        "west": "village_road",
        "north": "town_square",
        "east": "blacksmith",
        "south": "village_inn",
        "enter": "nexus_point_2",
        "northeast": "temple_of_architect",
        "northwest": "farm",
        "southeast": "general_store"
      }
    },
    {
      "id": "village_inn",
      "name": "Village Inn",
      "description": "A cozy, welcoming inn with a warm atmosphere. The common room is filled with comfortable chairs and tables, and a crackling fireplace provides warmth and light. The innkeeper, Lydia, greets guests with a warm smile. Rooms are available upstairs, and the inn serves hearty meals. The smell of fresh bread and cooking fills the air.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fireplace",
        "inn_table",
        "inn_chair"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "village_innkeeper",
          "name": "Lydia"
        }
      ],
      "exitCount": 2,
      "exits": {
        "north": "village_square",
        "up": "inn_rooms"
      }
    },
    {
      "id": "inn_rooms",
      "name": "Inn Rooms",
      "description": "A hallway with several guest rooms. Each room is simple but clean, with a bed, a small table, and a window. The rooms are well-maintained and provide a comfortable place to rest.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bed",
        "table",
        "window"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "village_inn"
      }
    },
    {
      "id": "temple_of_architect",
      "name": "Temple of The Architect",
      "description": "A beautiful, serene temple dedicated to The Architect. White marble columns support a vaulted ceiling, and symbols of The Architect adorn the walls. The air is filled with the scent of incense and the sound of soft prayers. The High Priestess Seraphina presides here, along with her paladin Marcus and missionaries Kira, Yuki, and Elena. The temple radiates peace and acceptance, welcoming all who seek The Architect's love.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "altar",
        "incense_burner",
        "prayer_cushions",
        "holy_symbols"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "priestess",
          "name": "Seraphina"
        },
        {
          "id": "paladin_marcus",
          "name": "Marcus"
        },
        {
          "id": "missionary_kira",
          "name": "Kira"
        },
        {
          "id": "missionary_yuki",
          "name": "Yuki"
        },
        {
          "id": "missionary_elena",
          "name": "Elena"
        }
      ],
      "exitCount": 3,
      "exits": {
        "southwest": "village_square",
        "east": "temple_garden",
        "north": "temple_training_grounds"
      }
    },
    {
      "id": "temple_garden",
      "name": "Temple Garden",
      "description": "A peaceful garden surrounding the temple. Beautiful flowers bloom in carefully tended beds, and a small fountain provides the gentle sound of flowing water. The garden is a place of meditation and reflection, where the priestess and missionaries often come to find peace and connect with The Architect's love.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "flowers",
        "fountain",
        "meditation_bench"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "temple_of_architect"
      }
    },
    {
      "id": "temple_training_grounds",
      "name": "Temple Training Grounds",
      "description": "A training area where the paladin Marcus practices and trains. Weapons racks line the walls, and training dummies stand ready. The area is well-maintained, reflecting the paladin's dedication to his duty of protection.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "weapon_rack",
        "training_dummy",
        "practice_weapons"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "temple_of_architect"
      }
    },
    {
      "id": "farm",
      "name": "Village Farm",
      "description": "A well-tended farm with fields of crops and pens for livestock. The farmer Jasper works the land with care, growing wheat, corn, potatoes, and carrots. Chickens peck at the ground, and a cow grazes peacefully. The farm provides fresh produce and livestock for the village.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "wheat_field",
        "corn_field",
        "chicken_coop",
        "cow_pen"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "farmer",
          "name": "Jasper"
        }
      ],
      "exitCount": 1,
      "exits": {
        "southeast": "village_square"
      }
    },
    {
      "id": "nexus_point_2",
      "name": "Nexus Point - Village Square",
      "description": "You stand before a monolithic Nexus Point in the village square. The ancient structure, carved from weathered stone and embedded with a large Elysium Crystal, towers above you. Glowing runes cover its surface, shifting and pulsing with interdimensional energy. The crystal at its core radiates a soft, ethereal light. This Nexus Point connects to other locations you've discovered, allowing instant travel between waypoints. To use it, simply trace the destination rune code in the air before the crystal.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "nexus_monolith",
        "elysium_crystal",
        "dimensional_runes"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "exit": "village_square"
      }
    },
    {
      "id": "blacksmith_waystone",
      "name": "Waystone - Blacksmith's Forge",
      "description": "You stand before a monolithic Waystone structure near the blacksmith's forge. Carved from dark stone and embedded with an Elysium Crystal, the waystone's runes glow with a warm, orange light that reflects the forge's heat. This modern waystone connects to other locations across the realm, allowing instant travel.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "waystone_monolith",
        "elysium_crystal",
        "glowing_runes"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "exit": "blacksmith"
      }
    },
    {
      "id": "general_store",
      "name": "General Store",
      "description": "A well-stocked general store filled with shelves of goods. The air smells of spices, dried goods, and fresh wood. Rows of barrels and crates line the walls, and a polished wooden counter runs along one side. The store owner keeps everything organized and tidy, with items clearly labeled and displayed. A small bell above the door chimes when customers enter.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "store_counter",
        "shelves",
        "barrels",
        "display_case"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "general_store_owner",
          "name": "Sam"
        }
      ],
      "exitCount": 2,
      "exits": {
        "south": "village_square",
        "northwest": "village_square"
      }
    },
    {
      "id": "blacksmith",
      "name": "Blacksmith's Forge",
      "description": "A hot, smoky forge filled with the sound of hammering metal. The air is thick with the smell of coal and heated iron. Anvils, hammers, and tongs are scattered around the workspace. The forge glows with intense heat, and sparks fly as the blacksmith works. Shelves line the walls, displaying finished weapons and armor. The blacksmith, a burly figure with powerful arms and a soot-stained apron, looks up from their work. Outside, a monolithic Waystone structure stands, its Elysium Crystal glowing with magical energy.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hammer",
        "tongs",
        "water_trough"
      ],
      "objects": [
        "forge",
        "anvil",
        "grindstone"
      ],
      "npcsHere": [
        {
          "id": "blacksmith",
          "name": "Riven"
        }
      ],
      "exitCount": 2,
      "exits": {
        "west": "village_square",
        "enter": "blacksmith_waystone"
      }
    },
    {
      "id": "forest_clearing",
      "name": "Forest Clearing",
      "description": "You emerge into a circular clearing surrounded by ancient oaks. In the center stands a moss-covered altar stone, etched with forgotten runes that seem to pulse with faint light. The clearing feels both ancient and alive, as if something powerful once happened here. A weathered journal lies on the altar.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "altar_stone",
        "journal",
        "mysterious_gem"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "south": "forest_path",
        "north": "mountain_pass",
        "east": "hidden_grove"
      }
    },
    {
      "id": "hidden_grove",
      "name": "Hidden Grove",
      "description": "A secret grove hidden deep in the forest. Ancient trees form a natural canopy overhead, and rare plants grow in abundance here. The air is thick with magical energy, and you notice strange glowing mushrooms dotting the forest floor. This place feels untouched by time.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rare_herbs",
        "glowing_mushrooms",
        "ancient_tree"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "forest_clearing"
      }
    },
    {
      "id": "hidden_cellar",
      "name": "Hidden Cellar",
      "description": "A secret cellar hidden beneath the house. The air is cool and musty, filled with the scent of old wine and preserved foods. Shelves line the walls, holding jars and bottles covered in dust. A single lantern provides dim light, casting long shadows across the stone floor.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "old_wine",
        "preserved_foods",
        "dusty_jars",
        "ancient_tome"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "west_of_house"
      }
    },
    {
      "id": "nexus_point_1",
      "name": "Nexus Point - Ancient Grove",
      "description": "A monolithic structure rises from the center of the grove, its surface covered in intricate runes that pulse with interdimensional energy. This is a Nexus Point - a convergence where the boundaries between dimensions blur. The structure hums with power, and you can sense other locations connected to this network. The runes shift and change, revealing destinations across the realm.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "nexus_monolith",
        "dimensional_rune",
        "energy_crystal"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "ancient_grove"
      }
    },
    {
      "id": "old_well",
      "name": "Old Well",
      "description": "You stand beside an old stone well, its wooden roof weathered and splintered. The well's opening is dark and you can barely make out your reflection in the water below. A rusty metal bucket sits on the stone rim, attached to a frayed rope. Something glints in the depths.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "well_bucket"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "garden"
      }
    },
    {
      "id": "shed",
      "name": "Garden Shed",
      "description": "A small wooden shed filled with gardening tools and supplies. Sunlight streams through a dusty window.",
      "locked": true,
      "region": "Hollow Ridge",
      "items": [
        "tools",
        "rope",
        "tension_wrench"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "east_of_house",
        "east": "abandoned_mill"
      }
    },
    {
      "id": "abandoned_mill",
      "name": "Abandoned Mill",
      "description": "An old water mill that has long since fallen into disrepair. The massive wooden water wheel is motionless, caked with mud and algae. The mill building itself has collapsed in places, revealing old machinery inside. Ivy has claimed much of the stone structure, but a ladder still provides access to the upper level. A buff, tough-looking miller works the machinery by the entrance.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "water_wheel",
        "rusty_gears",
        "ladder",
        "rope_pulley"
      ],
      "objects": [],
      "npcsHere": [
        {
          "id": "miller",
          "name": "Brenna"
        }
      ],
      "exitCount": 3,
      "exits": {
        "west": "shed",
        "east": "river_shore",
        "up": "mill_upper_level"
      }
    },
    {
      "id": "mill_upper_level",
      "name": "Mill Upper Level",
      "description": "The upper level of the mill is surprisingly intact. Dusty gears and machinery fill the space, their rusted teeth interlocking in ways that suggest complex engineering. A window offers a view of the river below. A sturdy workbench sits in one corner, tools organized with precision.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "miller_journal",
        "engineering_tools",
        "rope_pulley",
        "workbench"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "abandoned_mill"
      }
    },
    {
      "id": "river_shore",
      "name": "River Shore",
      "description": "The slow-moving river spreads out before you. A small wooden dock extends into the water, its planks warped and missing in several places. On the shore, an abandoned rowboat lies overturned, half-buried in mud. Strange symbols are carved into nearby trees, and you can see where the river bends south toward distant hills.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rowboat",
        "wooden_dock",
        "fishing_hook",
        "river_stones"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 3,
      "exits": {
        "west": "abandoned_mill",
        "east": "river_bend",
        "downstream": "hidden_cove"
      }
    },
    {
      "id": "hidden_cove",
      "name": "Hidden Cove",
      "description": "You've discovered a secluded cove, hidden behind a bend in the river. The water is clearer here, reflecting the overhanging trees. An ancient, partially submerged stone structure emerges from the water like a sleeping giant. Strange carvings cover its surface.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_temple_ruins",
        "crystal_pool",
        "carved_stone"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "upstream": "river_shore"
      }
    },
    {
      "id": "river_bend",
      "name": "River Bend",
      "description": "The river curves here, revealing a wider expanse of water and sandy banks. Ancient stone pillars rise from the water at regular intervals, suggesting some long-forgotten structure. A rope bridge spans one section, though many of its planks are missing. The far shore seems distant but reachable.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stone_pillars",
        "broken_rope_bridge",
        "river_treasure"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "river_shore",
        "east": "river_crossing"
      }
    },
    {
      "id": "river_crossing",
      "name": "River Crossing",
      "description": "You've reached a strategic point where the river is narrowest. A makeshift raft of logs is tied to a tree stump on the far bank. On this side, a cave opening is partially hidden by hanging vines. The sound of water rushing over rocks echoes from inside.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "makeshift_raft",
        "rope",
        "vines"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "river_bend",
        "east": "cave_entrance"
      }
    },
    {
      "id": "cellar",
      "name": "Cellar",
      "description": "A dark, stone-walled cellar with wine racks lining the walls. The air is cool and musty.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "wine_bottle"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "south_of_house"
      }
    },
    {
      "id": "treasure_room",
      "name": "Treasure Room",
      "description": "You have discovered a hidden treasure room! Golden light reflects off ancient artifacts and precious gems. This is the ultimate goal of your adventure!",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "treasure",
        "ancient_scroll"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "cave_depths"
      }
    },
    {
      "id": "mountain_pass",
      "name": "Mountain Pass",
      "description": "A narrow mountain pass winds through jagged cliffs. The air is thin and crisp here, and you can see for miles across the wilderness below. Strange rock formations jut from the mountainside, their surfaces marked with ancient symbols. A small cave entrance is visible further up the trail.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_carvings",
        "tinderbox",
        "obsidian_shard"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "forest_clearing",
        "south": "mountain_cave"
      }
    },
    {
      "id": "mountain_cave",
      "name": "Mountain Cave",
      "description": "The cave opens into a surprisingly large chamber. Stalactites hang like teeth from the ceiling, dripping water into shallow pools below. The walls are adorned with primitive cave paintings depicting strange creatures and symbols. A faint glow emanates from deeper within the cave. The air smells of stone and something else... something ancient.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "cave_painting",
        "mineral_crystals"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "mountain_pass",
        "east": "cave_depths"
      }
    },
    {
      "id": "cave_depths",
      "name": "Cave Depths",
      "description": "You venture deeper into the cave system. The glow grows stronger, revealing intricate patterns carved into the cave walls. The air hums with barely perceptible energy. In the center of the chamber, a large crystal formation pulses with soft blue light. Something valuable must be hidden here.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "glowing_crystal",
        "ancient_map",
        "mineral_ore"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "mountain_cave",
        "east": "treasure_room"
      }
    },
    {
      "id": "swamp_clearing",
      "name": "Swamp Clearing",
      "description": "The forest gives way to a murky swamp. Mist hangs low over stagnant water, and twisted trees protrude from the bog like skeletal hands. Strange sounds echo from the depths—croaking frogs, splashing, and something else you can't quite identify. A rickety wooden boardwalk extends into the swamp, though many planks are missing.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "bog_fungi",
        "twisted_root"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "forest_path",
        "east": "swamp_boardwalk"
      }
    },
    {
      "id": "swamp_boardwalk",
      "name": "Swamp Boardwalk",
      "description": "You carefully navigate the rotting boardwalk that winds through the swamp. The wood creaks ominously under your weight. Strange lights dance in the mist ahead—will-o'-wisps, perhaps? In the distance, an island rises from the murky water with a gnarled old tree at its center. Something glints among its roots.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rotten_plank",
        "marsh_gas"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "west": "swamp_clearing",
        "east": "swamp_island"
      }
    },
    {
      "id": "swamp_island",
      "name": "Swamp Island",
      "description": "You reach a small island in the center of the swamp. An ancient, gnarled tree dominates the space, its bark twisted and weathered. Thick roots expose something metallic buried beneath. The island feels significant, perhaps sacred. Strange energy emanates from the tree itself.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_tree",
        "mysterious_shovel"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "swamp_boardwalk"
      }
    },
    {
      "id": "hollow_ridge",
      "name": "Hollow Ridge",
      "description": "A narrow ridge of weathered stone that cuts through the landscape like a scar. The ridge is hollowed out in places, creating natural caves and overhangs. Ancient carvings mark the stone, their meaning lost to time. The air here feels thin and carries an otherworldly quality. A monolithic Waystone structure stands at the ridge's peak, its Elysium Crystal pulsing with soft light. The ridge offers a commanding view of Hollow Ridge below, with its castle, marketplace, and bustling town. Strange energies seem to gather here.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "weathered_stone",
        "ancient_carving",
        "crystal_fragment"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "south": "forest_path",
        "east": "village_road",
        "down": "hollow_ridge_cave",
        "north": "town_square"
      }
    },
    {
      "id": "hollow_ridge_cave",
      "name": "Hollow Ridge Cave",
      "description": "A natural cave formed within the hollow ridge. The walls are smooth and worn, suggesting this space has been used for centuries. Ancient symbols are carved into the walls, glowing faintly with residual magical energy. A small pool of clear water reflects the cave's ceiling, and strange crystals grow from the walls. The air here is still and sacred, as if this place holds great significance.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "cave_crystal",
        "ancient_symbol",
        "clear_water"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "hollow_ridge"
      }
    },
    {
      "id": "town_square",
      "name": "Town Square",
      "description": "The heart of Hollow Ridge, a bustling town square where paths converge. A magnificent fountain stands at the center, its waters sparkling in the light. A festival stage dominates one side, and a notice board displays various announcements. The square is surrounded by the various districts of the town, each with its own character and purpose. The kingdom's castle looms to the east, while the marketplace bustles to the south.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fountain_of_the_sky_rings",
        "festival_stage",
        "notice_board"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 14,
      "exits": {
        "north": "hollow_ridge",
        "south": "marketplace",
        "east": "castle",
        "west": "hollowridge_forest",
        "up": "onyx_apartment",
        "down": "lost_cave_dungeon",
        "southwest": "early_morning_coffee_shop",
        "southeast": "royal_gardens",
        "enter": "the_hidden_gem_tavern",
        "northwest": "residential_district",
        "northeast": "civic_center",
        "fountain": "fountain_of_the_sky_rings",
        "stage": "festival_stage",
        "board": "notice_board"
      }
    },
    {
      "id": "castle",
      "name": "Castle Entrance",
      "description": "The grand entrance to Hollow Ridge Castle. Massive stone walls rise high above, and the royal banner flutters in the breeze. Guards stand watch at the gate, their armor polished to a shine. The castle is an imposing structure, built to last centuries. Inside, you can see corridors leading to various parts of the castle.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "royal_banner",
        "castle_gate",
        "guard_post"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 9,
      "exits": {
        "west": "town_square",
        "enter": "throne_room",
        "north": "royal_library",
        "east": "war_council_hall",
        "south": "castle_kitchens",
        "down": "underground_vaults",
        "up": "servant_quarters",
        "northeast": "training_yard",
        "northwest": "castle_ramparts"
      }
    },
    {
      "id": "throne_room",
      "name": "Throne Room",
      "description": "A magnificent hall with vaulted ceilings and ornate tapestries. The royal throne sits at the far end on a raised dais, carved from ancient wood and inlaid with precious metals. Stained glass windows cast colorful light across the polished marble floor. This is where the ruler of Hollow Ridge holds court and makes important decisions.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "royal_throne",
        "ornate_tapestries",
        "stained_glass_windows"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "exit": "castle"
      }
    },
    {
      "id": "royal_library",
      "name": "Royal Library",
      "description": "A vast library filled with towering bookshelves that reach toward the ceiling. Ladders on rails allow access to the highest shelves. Ancient tomes, scrolls, and manuscripts fill every available space. The air smells of old paper and leather bindings. A reading area with comfortable chairs sits near a large fireplace.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_tomes",
        "scrolls",
        "reading_chairs",
        "fireplace"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "castle"
      }
    },
    {
      "id": "war_council_hall",
      "name": "War Council Hall",
      "description": "A strategic planning room with a large table covered in maps and tactical diagrams. Battle standards hang from the walls, and weapons are displayed in cases. This is where military strategies are discussed and important decisions about the kingdom's defense are made.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "war_table",
        "tactical_maps",
        "battle_standards",
        "weapon_display"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "castle"
      }
    },
    {
      "id": "castle_kitchens",
      "name": "Castle Kitchens",
      "description": "A massive kitchen with multiple hearths, ovens, and preparation areas. The air is filled with the aromas of cooking food. Cooks and kitchen staff work busily, preparing meals for the castle's residents. Copper pots and pans hang from the ceiling, and fresh ingredients are arranged on long tables.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "cooking_hearths",
        "copper_pots",
        "preparation_tables",
        "fresh_ingredients"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "castle"
      }
    },
    {
      "id": "underground_vaults",
      "name": "Underground Vaults",
      "description": "Deep beneath the castle, these vaults hold the kingdom's treasures, artifacts, and important documents. Heavy iron doors protect the most valuable items. Torches flicker on the walls, casting dancing shadows. The air is cool and dry, perfect for preservation.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_artifacts",
        "iron_doors",
        "torches"
      ],
      "objects": [
        "storage_chest"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "castle"
      }
    },
    {
      "id": "servant_quarters",
      "name": "Servant Quarters",
      "description": "The living quarters for the castle's staff. Simple but comfortable rooms line the corridors. The area is well-maintained and organized, reflecting the care the servants take in their work. Personal belongings and small comforts make these spaces feel like home.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "simple_beds",
        "personal_belongings",
        "storage_chests"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "castle"
      }
    },
    {
      "id": "training_yard",
      "name": "Training Yard",
      "description": "An open courtyard where guards and soldiers train. Practice dummies, weapon racks, and training equipment are arranged around the space. The sound of clashing steel and shouted commands fills the air. This is where the kingdom's defenders hone their skills.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "practice_dummies",
        "weapon_racks",
        "training_equipment"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "southwest": "castle"
      }
    },
    {
      "id": "castle_ramparts",
      "name": "Castle Ramparts",
      "description": "The walkway along the top of the castle walls. From here, you have a commanding view of the entire kingdom of Hollow Ridge. Guards patrol the ramparts, keeping watch for any threats. The wind is strong up here, and you can see for miles in every direction.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "watchtower",
        "battlements",
        "signal_flags"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "southeast": "castle",
        "up": "watchtower"
      }
    },
    {
      "id": "watchtower",
      "name": "Watchtower",
      "description": "The highest point of the castle, a tall watchtower that provides an unparalleled view of the surrounding lands. The tower is circular, with narrow windows on all sides. A spyglass sits on a small table, and a signal fire pit is ready to be lit. From here, you can see the entire kingdom spread out below like a map. The wind howls through the windows, and you feel as if you're standing at the edge of the world.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "spyglass",
        "signal_fire_pit",
        "observation_table"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "castle_ramparts"
      }
    },
    {
      "id": "hidden_shrine",
      "name": "Hidden Shrine",
      "description": "A small, secluded shrine hidden deep in the forest, almost completely overgrown with vines and moss. Ancient stone pillars form a circle around a central altar, and strange symbols are carved into the weathered stone. The air here feels different—charged with an ancient energy. Wildflowers grow in profusion around the shrine, and a small stream trickles nearby. This place feels sacred, untouched by time. The shrine seems dedicated to some forgotten deity, their name lost to the ages.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stone_altar",
        "ancient_pillars",
        "wildflowers",
        "stream"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "ancient_grove"
      }
    },
    {
      "id": "marketplace",
      "name": "Marketplace",
      "description": "A vibrant marketplace filled with stalls, carts, and merchants calling out their wares. The air is alive with the sounds of haggling, laughter, and commerce. Colorful awnings provide shade, and the ground is packed earth worn smooth by countless footsteps. This is the commercial heart of Hollow Ridge.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "market_stalls",
        "colorful_awnings",
        "merchant_carts"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 9,
      "exits": {
        "north": "town_square",
        "east": "fresh_goods_stalls",
        "west": "traveling_merchant_row",
        "south": "potion_and_alchemy_stand",
        "southeast": "artifact_trader",
        "southwest": "exotic_pet_corner",
        "northeast": "bakery_cart",
        "northwest": "fortune_teller_tent",
        "down": "hollowridge_docks"
      }
    },
    {
      "id": "fresh_goods_stalls",
      "name": "Fresh Goods Stalls",
      "description": "Stalls overflowing with fresh produce, fruits, vegetables, and locally grown foods. The vibrant colors and fresh scents are inviting. Farmers and vendors display their best wares, calling out prices and specials to passersby.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fresh_fruits",
        "vegetables",
        "produce_baskets"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "marketplace"
      }
    },
    {
      "id": "traveling_merchant_row",
      "name": "Traveling Merchant Row",
      "description": "A row of stalls set up by traveling merchants from distant lands. Exotic goods, rare items, and unusual wares fill the displays. These merchants bring news from far away and offer items not found elsewhere in the kingdom.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "exotic_goods",
        "rare_items",
        "traveling_merchant_stalls"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "marketplace"
      }
    },
    {
      "id": "potion_and_alchemy_stand",
      "name": "Potion and Alchemy Stand",
      "description": "A specialized stall filled with potions, elixirs, and alchemical ingredients. Glass bottles of various colors line the shelves, and the air carries the scent of herbs and magical reagents. An alchemist tends the stand, ready to sell or create custom potions.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "potions",
        "elixirs",
        "alchemical_ingredients",
        "glass_bottles"
      ],
      "objects": [
        "alchemy_table",
        "cauldron"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "marketplace"
      }
    },
    {
      "id": "artifact_trader",
      "name": "Artifact Trader",
      "description": "A mysterious stall dealing in ancient artifacts, magical items, and rare curiosities. The trader is knowledgeable about the history and properties of each item. Some items glow with residual magic, while others are simply beautiful or historically significant.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_artifacts",
        "magical_items",
        "rare_curiosities"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "northwest": "marketplace"
      }
    },
    {
      "id": "exotic_pet_corner",
      "name": "Exotic Pet Corner",
      "description": "A section of the marketplace dedicated to exotic animals and pets. Cages and enclosures hold various creatures, from colorful birds to small magical beasts. The pet merchant knows how to care for each animal and can help match the right pet to the right owner.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "pet_cages",
        "exotic_animals",
        "pet_supplies"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "northeast": "marketplace"
      }
    },
    {
      "id": "bakery_cart",
      "name": "Bakery Cart",
      "description": "A mobile bakery cart offering fresh bread, pastries, and sweet treats. The aroma of baking bread is irresistible. The baker works from early morning, ensuring everything is fresh and delicious.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fresh_bread",
        "pastries",
        "sweet_treats"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "southwest": "marketplace"
      }
    },
    {
      "id": "fortune_teller_tent",
      "name": "Fortune Teller Tent",
      "description": "A colorful tent where a fortune teller offers readings and predictions. Mystical symbols decorate the fabric, and the interior is dimly lit with candles. Crystal balls, tarot cards, and other divination tools are arranged on a small table.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "crystal_ball",
        "tarot_cards",
        "mystical_symbols",
        "candles"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "southeast": "marketplace"
      }
    },
    {
      "id": "residential_district",
      "name": "Residential District",
      "description": "A neighborhood of homes where the citizens of Hollow Ridge live. The streets are clean and well-maintained, with gardens and small yards. Children play in the streets, and neighbors chat on their doorsteps. This is a peaceful, community-oriented area.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "residential_homes",
        "community_gardens",
        "street_lamps"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "north": "row_houses",
        "east": "stonebridge_homes",
        "south": "lower_town_apartments",
        "west": "town_square"
      }
    },
    {
      "id": "row_houses",
      "name": "Row Houses",
      "description": "A line of connected houses built in a uniform style. Each home has its own character, with different decorations and gardens. The residents take pride in their homes, and the area feels welcoming and safe.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "connected_houses",
        "decorative_gardens"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "residential_district"
      }
    },
    {
      "id": "stonebridge_homes",
      "name": "Stonebridge Homes",
      "description": "Homes built near a stone bridge that crosses a small stream. These houses are slightly larger and more well-appointed, with stone foundations and quality construction. The bridge itself is a local landmark.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "stone_bridge",
        "quality_homes",
        "stream"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "residential_district"
      }
    },
    {
      "id": "lower_town_apartments",
      "name": "Lower Town Apartments",
      "description": "Multi-story apartment buildings that provide affordable housing. The buildings are well-maintained, and the community is diverse. Small shops and services are located on the ground floors, making this a self-contained neighborhood.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "apartment_buildings",
        "ground_floor_shops"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "residential_district"
      }
    },
    {
      "id": "civic_center",
      "name": "Civic Center",
      "description": "The administrative heart of Hollow Ridge. Important buildings cluster here, including the town hall, community board, and guard outpost. This is where citizens come to conduct official business, report issues, and stay informed about community matters.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "administrative_buildings",
        "official_notices"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "north": "town_hall",
        "east": "community_board",
        "south": "guard_outpost",
        "west": "town_square"
      }
    },
    {
      "id": "town_hall",
      "name": "Town Hall",
      "description": "A stately building where the town's administration is conducted. Official meetings, public forums, and important announcements happen here. The building is well-maintained and serves as a symbol of the community's organization and governance.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "meeting_hall",
        "official_documents",
        "public_forum"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "civic_center"
      }
    },
    {
      "id": "community_board",
      "name": "Community Board",
      "description": "A public space with notice boards displaying announcements, job postings, community events, and local news. Citizens gather here to stay informed and connect with their neighbors. It's a hub of community information.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "notice_boards",
        "announcements",
        "job_postings"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "civic_center"
      }
    },
    {
      "id": "guard_outpost",
      "name": "Guard Outpost",
      "description": "A small station where the town guards are based. Guards can be seen coming and going, and citizens can report crimes or request assistance here. The outpost is well-equipped and staffed by professional guards dedicated to keeping the town safe.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "guard_station",
        "weapon_racks",
        "report_desk"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "civic_center"
      }
    },
    {
      "id": "craftsman_quarter",
      "name": "Craftsman Quarter",
      "description": "A district dedicated to skilled craftspeople. Workshops and studios line the streets, each specializing in different trades. The sound of hammers, saws, and other tools fills the air. This is where quality goods are made by hand.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "workshops",
        "craft_studios",
        "tool_sounds"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "north": "blacksmith_forge",
        "east": "tailor_shop",
        "south": "woodworkers_guild",
        "west": "town_square"
      }
    },
    {
      "id": "blacksmith_forge",
      "name": "Blacksmith Forge",
      "description": "A professional blacksmith's workshop in the craftsman quarter. The forge burns hot, and the smith works on various projects. This is a larger, more established operation than the village blacksmith, serving the needs of the entire kingdom.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "smithing_tools"
      ],
      "objects": [
        "forge",
        "anvil",
        "grindstone"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "craftsman_quarter"
      }
    },
    {
      "id": "tailor_shop",
      "name": "Tailor Shop",
      "description": "A fine tailoring establishment where skilled seamstresses and tailors create custom clothing. Fabrics of all kinds line the walls, and finished garments are displayed. The shop offers both everyday wear and special occasion attire.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fabrics",
        "finished_garments"
      ],
      "objects": [
        "loom",
        "spinning_wheel",
        "workbench"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "craftsman_quarter"
      }
    },
    {
      "id": "woodworkers_guild",
      "name": "Woodworkers Guild",
      "description": "A guild hall and workshop for skilled woodworkers. Furniture, tools, and decorative items are crafted here. The scent of sawdust and wood polish fills the air. Master craftspeople train apprentices in the art of woodworking.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "furniture",
        "wood_supplies"
      ],
      "objects": [
        "sawmill",
        "workbench",
        "storage_chest"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "craftsman_quarter"
      }
    },
    {
      "id": "hollowridge_docks",
      "name": "Hollowridge Docks",
      "description": "A bustling dock area where boats come and go. The docks are well-maintained, with piers extending into the water. Fishermen, traders, and travelers use these facilities. The water is clear, and you can see fish swimming below.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "docks",
        "boats",
        "fishing_equipment"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 4,
      "exits": {
        "north": "fishing_piers",
        "east": "ferry_crossing",
        "south": "smugglers_slip",
        "west": "town_square"
      }
    },
    {
      "id": "fishing_piers",
      "name": "Fishing Piers",
      "description": "Long piers extending into the water where fishermen cast their lines. The piers are popular with both professional fishermen and hobbyists. The water is teeming with fish, and it's a peaceful place to spend time.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "fishing_piers",
        "fishing_equipment",
        "fish_baskets"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "hollowridge_docks"
      }
    },
    {
      "id": "ferry_crossing",
      "name": "Ferry Crossing",
      "description": "A ferry service that crosses the water to other locations. The ferry is a reliable means of transportation, and the ferryman knows the waters well. Travelers wait here for the next crossing.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ferry_boat",
        "ferryman_station",
        "waiting_area"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "hollowridge_docks"
      }
    },
    {
      "id": "smugglers_slip",
      "name": "Smugglers Slip",
      "description": "A hidden, less-traveled area of the docks. Rumors suggest this is where less legitimate activities take place. The area is shadowy and quiet, with hidden nooks and crannies. Those who know where to look can find interesting opportunities here.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hidden_nooks",
        "shadowy_corners"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "hollowridge_docks"
      }
    },
    {
      "id": "fountain_of_the_sky_rings",
      "name": "Fountain of the Sky Rings",
      "description": "A magnificent fountain at the center of the town square. The fountain features rings that seem to float in the air, with water cascading through them in impossible ways. It's a magical marvel that draws visitors and serves as a gathering point.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "magical_fountain",
        "floating_rings",
        "cascading_water"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "north": "town_square",
        "square": "town_square"
      }
    },
    {
      "id": "festival_stage",
      "name": "Festival Stage",
      "description": "A permanent stage built for performances, festivals, and community events. The stage is well-maintained and can accommodate large productions. During festivals, it's the center of entertainment and celebration.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "performance_stage",
        "festival_decorations"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "south": "town_square",
        "square": "town_square"
      }
    },
    {
      "id": "notice_board",
      "name": "Notice Board",
      "description": "A large board in the town square where announcements, job postings, and community information are displayed. Citizens regularly check here for news and opportunities. The board is well-organized and kept up to date.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "notice_board",
        "announcements",
        "job_postings"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 2,
      "exits": {
        "east": "town_square",
        "square": "town_square"
      }
    },
    {
      "id": "early_morning_coffee_shop",
      "name": "Early Morning Coffee Shop",
      "description": "A cozy coffee shop that opens before dawn, serving the earliest risers. The aroma of freshly roasted coffee fills the air, and the atmosphere is warm and inviting. This is a favorite spot for those who start their day early.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "coffee_bar",
        "fresh_roasts",
        "cozy_atmosphere"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 5,
      "exits": {
        "northeast": "town_square",
        "up": "rooftop_sipping_area",
        "east": "cozy_booths",
        "west": "roasting_room",
        "south": "hidden_back_alcove"
      }
    },
    {
      "id": "rooftop_sipping_area",
      "name": "Rooftop Sipping Area",
      "description": "An open-air rooftop area where customers can enjoy their coffee while taking in the view of Hollow Ridge. The morning air is fresh, and the sunrise views are spectacular. This is a peaceful, elevated space.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rooftop_seating",
        "morning_views"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "early_morning_coffee_shop"
      }
    },
    {
      "id": "cozy_booths",
      "name": "Cozy Booths",
      "description": "Private booths within the coffee shop, perfect for quiet conversation or solitary reflection. The booths are comfortable and well-designed, with soft lighting and privacy.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "private_booths",
        "comfortable_seating"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "early_morning_coffee_shop"
      }
    },
    {
      "id": "roasting_room",
      "name": "Roasting Room",
      "description": "The back room where coffee beans are roasted. The process fills the air with rich, aromatic scents. The roaster takes pride in their craft, carefully controlling temperature and timing to create the perfect roast.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "coffee_beans"
      ],
      "objects": [
        "furnace",
        "workbench",
        "storage_chest"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "early_morning_coffee_shop"
      }
    },
    {
      "id": "hidden_back_alcove",
      "name": "Hidden Back Alcove",
      "description": "A secluded alcove at the back of the coffee shop, hidden from the main area. This is a quiet, private space where sensitive conversations can take place, or where one can simply enjoy solitude.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hidden_alcove",
        "private_seating"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "early_morning_coffee_shop"
      }
    },
    {
      "id": "royal_gardens",
      "name": "Royal Gardens",
      "description": "Magnificent gardens maintained for the royal family and open to the public. The gardens are beautifully landscaped with paths winding through various themed areas. Flowers bloom in abundance, and the air is filled with their fragrance.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "garden_paths",
        "flowering_plants",
        "landscaped_areas"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 6,
      "exits": {
        "northwest": "town_square",
        "north": "crystal_pond",
        "east": "maze_of_reflection",
        "south": "rose_archway",
        "west": "songbird_grove",
        "southwest": "the_hidden_bridge"
      }
    },
    {
      "id": "crystal_pond",
      "name": "Crystal Pond",
      "description": "A serene pond with crystal-clear water that reflects the sky perfectly. Koi fish swim lazily beneath the surface, and water lilies float on top. A small bridge crosses the pond, and benches offer places to sit and contemplate.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "crystal_pond",
        "koi_fish",
        "water_lilies",
        "pond_bridge"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "royal_gardens"
      }
    },
    {
      "id": "maze_of_reflection",
      "name": "Maze of Reflection",
      "description": "A hedge maze designed for contemplation and meditation. The paths wind in intricate patterns, and mirrors placed at strategic points create interesting visual effects. Those who navigate it often find themselves reflecting on life's journey.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hedge_maze",
        "reflection_mirrors",
        "contemplation_benches"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "royal_gardens"
      }
    },
    {
      "id": "rose_archway",
      "name": "Rose Archway",
      "description": "A beautiful archway covered in climbing roses. The flowers bloom in various colors, creating a stunning natural tunnel. The scent is intoxicating, and the archway leads to a special garden area beyond.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rose_archway",
        "climbing_roses",
        "garden_tunnel"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "royal_gardens"
      }
    },
    {
      "id": "songbird_grove",
      "name": "Songbird Grove",
      "description": "A grove of trees where songbirds make their home. The area is filled with the beautiful sounds of birdsong throughout the day. Benches are placed among the trees, allowing visitors to sit and enjoy the natural music.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "songbird_trees",
        "bird_songs",
        "grove_benches"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "royal_gardens"
      }
    },
    {
      "id": "the_hidden_bridge",
      "name": "The Hidden Bridge",
      "description": "A small, hidden bridge that crosses a stream within the gardens. The bridge is partially obscured by overhanging plants, making it easy to miss. It leads to a secluded area of the gardens that few visitors discover.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hidden_bridge",
        "overhanging_plants",
        "secluded_area"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "northeast": "royal_gardens"
      }
    },
    {
      "id": "hollowridge_forest",
      "name": "Hollowridge Forest",
      "description": "A mystical forest that borders the kingdom. The trees are ancient and tall, and the forest has an otherworldly quality. Strange sounds and lights can sometimes be seen or heard among the trees. This is a place of mystery and natural beauty.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_trees",
        "forest_paths",
        "mystical_atmosphere"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 7,
      "exits": {
        "east": "town_square",
        "north": "whispering_trees_path",
        "south": "moonlight_clearing",
        "west": "mushroom_circle",
        "northwest": "abandoned_hunters_cabin",
        "northeast": "waterfall_of_echoes",
        "southwest": "ancient_forest_gate"
      }
    },
    {
      "id": "whispering_trees_path",
      "name": "Whispering Trees Path",
      "description": "A path through the forest where the trees seem to whisper secrets. The wind through the leaves creates sounds that almost sound like voices. Some say the trees themselves are trying to communicate with those who pass by.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "whispering_trees",
        "mystical_path"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "hollowridge_forest"
      }
    },
    {
      "id": "moonlight_clearing",
      "name": "Moonlight Clearing",
      "description": "A clearing in the forest that seems to catch and reflect moonlight in unusual ways. On clear nights, the clearing glows with a soft, silvery light. It's a peaceful, magical place that draws those seeking solitude or inspiration.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "moonlight_clearing",
        "silvery_light"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "hollowridge_forest"
      }
    },
    {
      "id": "mushroom_circle",
      "name": "Mushroom Circle",
      "description": "A circle of large, colorful mushrooms that grows in a perfect ring. The mushrooms glow faintly with bioluminescent light, and the area feels charged with magical energy. Some believe this is a place where the veil between worlds is thin.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "mushroom_circle",
        "glowing_mushrooms",
        "magical_energy"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "hollowridge_forest"
      }
    },
    {
      "id": "abandoned_hunters_cabin",
      "name": "Abandoned Hunters Cabin",
      "description": "An old cabin that was once used by hunters. It's been abandoned for some time, but the structure is still sound. Inside, you can find old equipment, furniture, and perhaps some forgotten supplies. The cabin has a mysterious, lonely atmosphere.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "abandoned_furniture"
      ],
      "objects": [
        "tanning_rack",
        "workbench",
        "storage_chest"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "southeast": "hollowridge_forest"
      }
    },
    {
      "id": "waterfall_of_echoes",
      "name": "Waterfall of Echoes",
      "description": "A beautiful waterfall where the sound echoes in unusual ways. The water cascades down a rocky face into a pool below. The echoes create a musical quality, and some say you can hear voices or messages in the sounds if you listen carefully.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "waterfall",
        "echoing_sounds",
        "pool"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "southwest": "hollowridge_forest"
      }
    },
    {
      "id": "ancient_forest_gate",
      "name": "Ancient Forest Gate",
      "description": "An ancient stone gate that stands at the edge of the forest. The gate is covered in moss and carvings that are mostly worn away by time. It's unclear what the gate was meant to guard or mark, but it has an ancient, powerful presence.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_gate",
        "stone_carvings",
        "moss_covered_stone"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "northeast": "hollowridge_forest"
      }
    },
    {
      "id": "lost_cave_dungeon",
      "name": "Lost Cave Dungeon",
      "description": "The entrance to a mysterious cave system that extends deep underground. The entrance is partially hidden and requires some exploration to find. Inside, the caves branch into various chambers and passages, some natural and some clearly worked by ancient hands.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "cave_entrance",
        "ancient_passages"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 7,
      "exits": {
        "up": "town_square",
        "north": "crystal_catacombs",
        "east": "underground_lake",
        "south": "collapsed_mineshaft",
        "west": "cursed_altar_chamber",
        "down": "forgotten_armory",
        "southwest": "deep_sanctum"
      }
    },
    {
      "id": "crystal_catacombs",
      "name": "Crystal Catacombs",
      "description": "Catacombs where the walls are embedded with glowing crystals. The crystals provide a soft, ethereal light that illuminates the passages. Ancient burial niches line the walls, and the air is cool and still. This is a place of both beauty and solemnity.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "glowing_crystals",
        "burial_niches",
        "crystal_embedded_walls"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "lost_cave_dungeon"
      }
    },
    {
      "id": "underground_lake",
      "name": "Underground Lake",
      "description": "A vast underground lake with still, dark water. The lake is deep and mysterious, and strange things are said to live in its depths. The water reflects the light from above, creating an otherworldly atmosphere. A small boat is tied to a makeshift dock.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "underground_lake",
        "dark_water",
        "small_boat"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "lost_cave_dungeon"
      }
    },
    {
      "id": "collapsed_mineshaft",
      "name": "Collapsed Mineshaft",
      "description": "An old mineshaft that has partially collapsed. Rubble blocks some passages, but others remain accessible. Old mining equipment lies abandoned, and the area is dangerous but potentially rewarding for those willing to explore carefully.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rubble"
      ],
      "objects": [
        "smelter",
        "furnace"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "lost_cave_dungeon"
      }
    },
    {
      "id": "cursed_altar_chamber",
      "name": "Cursed Altar Chamber",
      "description": "A chamber containing an ancient altar that radiates dark energy. The altar is covered in strange symbols and appears to have been used for dark rituals. The air feels heavy and oppressive, and those who stay too long may feel uneasy.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "dark_symbols",
        "ritual_markers"
      ],
      "objects": [
        "enchanting_table",
        "cauldron"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "lost_cave_dungeon"
      }
    },
    {
      "id": "forgotten_armory",
      "name": "Forgotten Armory",
      "description": "A hidden armory deep in the caves, filled with old weapons and armor. The equipment is ancient but well-preserved, suggesting it was stored carefully. Some pieces may still be functional, while others are valuable as historical artifacts.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "ancient_weapons",
        "old_armor"
      ],
      "objects": [
        "storage_chest",
        "grindstone",
        "anvil"
      ],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "up": "lost_cave_dungeon"
      }
    },
    {
      "id": "deep_sanctum",
      "name": "Deep Sanctum",
      "description": "The deepest part of the cave system, a sanctum that feels sacred and ancient. The chamber is circular, with symbols carved into the walls that glow with a soft light. This is clearly a place of great significance, though its exact purpose has been lost to time.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "sanctum_chamber",
        "glowing_symbols",
        "ancient_carvings"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "northeast": "lost_cave_dungeon"
      }
    },
    {
      "id": "the_hidden_gem_tavern",
      "name": "The Hidden Gem Tavern",
      "description": "A popular tavern known for its welcoming atmosphere and quality drinks. The tavern is well-frequented by locals and travelers alike. The main hall is lively, with music, conversation, and good food. It's a place where stories are shared and friendships are made.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "tables",
        "warm_atmosphere"
      ],
      "objects": [
        "cooking_station",
        "distillery",
        "storage_chest"
      ],
      "npcsHere": [],
      "exitCount": 6,
      "exits": {
        "north": "town_square",
        "enter": "main_hall",
        "east": "private_rooms",
        "west": "cellar_tunnel",
        "south": "secret_back_room",
        "up": "bard_stage"
      }
    },
    {
      "id": "main_hall",
      "name": "Main Hall",
      "description": "The main hall of The Hidden Gem Tavern. The room is spacious and welcoming, with tables arranged for groups of various sizes. A bar runs along one side, and a fireplace provides warmth and ambiance. This is where most of the tavern's activity takes place.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "main_tables",
        "tavern_bar",
        "fireplace"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "exit": "the_hidden_gem_tavern"
      }
    },
    {
      "id": "private_rooms",
      "name": "Private Rooms",
      "description": "Private rooms available for rent in the tavern. These rooms offer privacy and comfort for travelers who need a place to stay. The rooms are clean, well-furnished, and provide a good night's rest.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "private_rooms",
        "comfortable_beds",
        "traveler_accommodations"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "the_hidden_gem_tavern"
      }
    },
    {
      "id": "cellar_tunnel",
      "name": "Cellar Tunnel",
      "description": "A tunnel that leads from the tavern's cellar to other parts of the building or even beyond. The tunnel is dimly lit and has an air of mystery. Some say it connects to other locations in the town, though its exact purpose is unclear.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "cellar_tunnel",
        "dim_lighting",
        "mysterious_passage"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "the_hidden_gem_tavern"
      }
    },
    {
      "id": "secret_back_room",
      "name": "Secret Back Room",
      "description": "A hidden room at the back of the tavern, accessible only to those who know where to look. This room is used for private meetings, sensitive discussions, or simply as a quiet retreat from the main hall's noise.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "secret_room",
        "private_meeting_space"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "the_hidden_gem_tavern"
      }
    },
    {
      "id": "bard_stage",
      "name": "Bard Stage",
      "description": "A stage where bards and musicians perform for the tavern's patrons. The stage is well-designed for acoustics, and performances here are always popular. Talented musicians from near and far come to perform on this stage.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "performance_stage",
        "musical_instruments",
        "acoustic_design"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "the_hidden_gem_tavern"
      }
    },
    {
      "id": "onyx_apartment",
      "name": "Onyx Apartment",
      "description": "A luxurious apartment building known for its high-quality accommodations. The building is well-maintained and offers premium living spaces. The Onyx Apartment is a prestigious address in Hollow Ridge.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "luxury_apartment",
        "premium_accommodations"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 6,
      "exits": {
        "down": "town_square",
        "north": "balcony_view",
        "east": "main_living_room",
        "south": "sealed_room",
        "west": "rooftop_access",
        "up": "hidden_floor_panel"
      }
    },
    {
      "id": "balcony_view",
      "name": "Balcony View",
      "description": "A balcony that offers a stunning view of Hollow Ridge. From here, you can see the entire kingdom spread out below - the castle, marketplace, town, and surrounding areas. The view is especially beautiful at sunrise and sunset.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "balcony",
        "kingdom_view"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "south": "onyx_apartment"
      }
    },
    {
      "id": "main_living_room",
      "name": "Main Living Room",
      "description": "The main living space of the Onyx Apartment. The room is elegantly furnished with quality furniture and tasteful decorations. Large windows let in natural light, and the space feels both luxurious and comfortable.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "elegant_furniture",
        "large_windows",
        "tasteful_decorations"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "west": "onyx_apartment"
      }
    },
    {
      "id": "sealed_room",
      "name": "Sealed Room",
      "description": "A room that has been sealed off, its door locked and perhaps even bricked up. The reason for the sealing is unclear, but it adds an air of mystery to the apartment. Some say the room is haunted, while others believe it contains something valuable or dangerous.",
      "locked": true,
      "region": "Hollow Ridge",
      "items": [
        "sealed_door",
        "mysterious_room"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "north": "onyx_apartment"
      }
    },
    {
      "id": "rooftop_access",
      "name": "Rooftop Access",
      "description": "Access to the rooftop of the Onyx Apartment building. The rooftop offers panoramic views and is a peaceful place to relax. Some residents use it for gardening, stargazing, or simply enjoying the fresh air.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "rooftop",
        "panoramic_views",
        "rooftop_garden"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "east": "onyx_apartment"
      }
    },
    {
      "id": "hidden_floor_panel",
      "name": "Hidden Floor Panel",
      "description": "A hidden floor panel that can be opened to reveal a secret space below. The panel is well-concealed and requires knowledge or careful searching to discover. What lies beneath is a mystery, but it's clearly meant to be hidden.",
      "locked": false,
      "region": "Hollow Ridge",
      "items": [
        "hidden_panel",
        "secret_space"
      ],
      "objects": [],
      "npcsHere": [],
      "exitCount": 1,
      "exits": {
        "down": "onyx_apartment"
      }
    }
  ],
  "npcs": [
    {
      "id": "deer",
      "name": "deer",
      "currentLocation": null,
      "scheduleLocations": [],
      "canGoNow": []
    },
    {
      "id": "rabbit",
      "name": "rabbit",
      "currentLocation": null,
      "scheduleLocations": [],
      "canGoNow": []
    },
    {
      "id": "wolf",
      "name": "Wolf",
      "currentLocation": "forest_path",
      "scheduleLocations": [
        "forest_path",
        "swamp_clearing"
      ],
      "canGoNow": [
        "north_of_house",
        "forest_clearing",
        "swamp_clearing",
        "village_road",
        "deep_forest",
        "meadow"
      ]
    },
    {
      "id": "miller",
      "name": "Brenna",
      "currentLocation": "abandoned_mill",
      "scheduleLocations": [
        "abandoned_mill",
        "mill_upper_level",
        "river_shore"
      ],
      "canGoNow": [
        "shed",
        "river_shore",
        "mill_upper_level"
      ]
    },
    {
      "id": "blacksmith",
      "name": "Riven",
      "currentLocation": "blacksmith",
      "scheduleLocations": [
        "blacksmith"
      ],
      "canGoNow": [
        "village_square",
        "blacksmith_waystone"
      ]
    },
    {
      "id": "tavern_keeper",
      "name": "Soren",
      "currentLocation": "tavern_common_room",
      "scheduleLocations": [
        "tavern_common_room",
        "tavern_kitchen",
        "tavern_back_room",
        "tavern_private_room"
      ],
      "canGoNow": [
        "tavern_exterior",
        "tavern_kitchen",
        "tavern_back_room",
        "tavern_stairs",
        "tavern_privy"
      ]
    },
    {
      "id": "bartender",
      "name": "Silas (The Bartender)",
      "currentLocation": "tavern_common_room",
      "scheduleLocations": [
        "tavern_common_room"
      ],
      "canGoNow": [
        "tavern_exterior",
        "tavern_kitchen",
        "tavern_back_room",
        "tavern_stairs",
        "tavern_privy"
      ]
    },
    {
      "id": "traveling_merchant",
      "name": "Corbin",
      "currentLocation": "west_of_house",
      "scheduleLocations": [
        "west_of_house",
        "village_square",
        "tavern_exterior",
        "forest_path",
        "village_road"
      ],
      "canGoNow": [
        "north_of_house",
        "south_of_house",
        "front_door",
        "hidden_cellar"
      ]
    },
    {
      "id": "general_store_owner",
      "name": "Sam",
      "currentLocation": "general_store",
      "scheduleLocations": [
        "general_store",
        "village_square"
      ],
      "canGoNow": [
        "village_square"
      ]
    },
    {
      "id": "priestess",
      "name": "Seraphina",
      "currentLocation": "temple_of_architect",
      "scheduleLocations": [
        "temple_of_architect",
        "temple_garden"
      ],
      "canGoNow": [
        "village_square",
        "temple_garden",
        "temple_training_grounds"
      ]
    },
    {
      "id": "paladin_marcus",
      "name": "Marcus",
      "currentLocation": "temple_of_architect",
      "scheduleLocations": [
        "temple_training_grounds",
        "temple_of_architect",
        "temple_garden"
      ],
      "canGoNow": [
        "village_square",
        "temple_garden",
        "temple_training_grounds"
      ]
    },
    {
      "id": "missionary_kira",
      "name": "Kira",
      "currentLocation": "temple_of_architect",
      "scheduleLocations": [
        "temple_of_architect",
        "village_square",
        "temple_garden"
      ],
      "canGoNow": [
        "village_square",
        "temple_garden",
        "temple_training_grounds"
      ]
    },
    {
      "id": "missionary_yuki",
      "name": "Yuki",
      "currentLocation": "temple_of_architect",
      "scheduleLocations": [
        "temple_of_architect",
        "village_square",
        "temple_garden"
      ],
      "canGoNow": [
        "village_square",
        "temple_garden",
        "temple_training_grounds"
      ]
    },
    {
      "id": "missionary_elena",
      "name": "Elena",
      "currentLocation": "temple_of_architect",
      "scheduleLocations": [
        "temple_of_architect",
        "village_square",
        "temple_garden"
      ],
      "canGoNow": [
        "village_square",
        "temple_garden",
        "temple_training_grounds"
      ]
    },
    {
      "id": "village_innkeeper",
      "name": "Lydia",
      "currentLocation": "village_inn",
      "scheduleLocations": [
        "village_inn"
      ],
      "canGoNow": [
        "village_square",
        "inn_rooms"
      ]
    },
    {
      "id": "forest_hermit",
      "name": "Thorn",
      "currentLocation": "deep_forest",
      "scheduleLocations": [
        "deep_forest",
        "ancient_grove"
      ],
      "canGoNow": [
        "forest_path",
        "forest_clearing",
        "ancient_grove",
        "hermit_hut"
      ]
    },
    {
      "id": "traveling_bard",
      "name": "Aria",
      "currentLocation": "village_square",
      "scheduleLocations": [
        "village_square",
        "tavern_common_room"
      ],
      "canGoNow": [
        "village_road",
        "town_square",
        "blacksmith",
        "village_inn",
        "nexus_point_2",
        "temple_of_architect",
        "farm",
        "general_store"
      ]
    },
    {
      "id": "village_guard",
      "name": "Garrett",
      "currentLocation": "village_square",
      "scheduleLocations": [
        "village_square",
        "tavern_exterior"
      ],
      "canGoNow": [
        "village_road",
        "town_square",
        "blacksmith",
        "village_inn",
        "nexus_point_2",
        "temple_of_architect",
        "farm",
        "general_store"
      ]
    },
    {
      "id": "farmer",
      "name": "Jasper",
      "currentLocation": "farm",
      "scheduleLocations": [
        "farm",
        "village_square"
      ],
      "canGoNow": [
        "village_square"
      ]
    }
  ]
};
