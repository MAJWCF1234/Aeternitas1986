// Universal NPC Character Mod Template (Advanced)
// Load in game with: loadmod
// Focus: compatible with the extended topic-branch dialogue system.

const MOD_ID = "universal_npc_template";
const ID = (x) => `${MOD_ID}:${x}`;

const NPC_ID = ID("new_character");
const NPC_ROOM = "village_square"; // Must be a real room id.

const npcSkeleton = modAPI.createNPCCharacterTemplate({
  name: "New Character",
  currentLocation: NPC_ROOM
});

const BEHAVIORS = {
  observing_crowd: {
    description: "They quietly study everyone passing by.",
    dialogue: "Mornings tell you who is honest and who is late."
  },
  trading_tips: {
    description: "They trade practical advice with locals.",
    dialogue: "Midday deals are cleaner. Less desperation in the price."
  },
  watchful_patrol: {
    description: "They keep an eye on alleys and exits as evening closes in.",
    dialogue: "When the light fades, exits matter more than promises."
  }
};

const SCHEDULE = [
  { time: 6, location: NPC_ROOM, behaviors: ["observing_crowd"] },
  { time: 12, location: NPC_ROOM, behaviors: ["trading_tips"] },
  { time: 18, location: NPC_ROOM, behaviors: ["watchful_patrol"] }
];

const TOPIC_BRANCHES = {
  background: {
    start: {
      response: "Background lane. Pick one: origin, turning point, or values.",
      next: [
        { keywords: ["origin", "past"], response: "I grew up on poor roads and learned to read intent quickly.", setFlag: "heard_origin", nextNode: "background_follow" },
        { keywords: ["turning", "event"], response: "One bad contract taught me that caution beats speed.", setFlag: "heard_turning_point", nextNode: "background_follow" },
        { keywords: ["values", "principles"], response: "I respect consistency, plain words, and clean exits.", setFlag: "heard_values", nextNode: "background_follow" }
      ]
    },
    background_follow: {
      response: "Want details or practical advice from it?",
      next: [
        { keywords: ["details", "detail"], response: "Details cost trust. Keep showing up and I will share more.", rel: { friendship: 1, romance: 0 }, end: true },
        { keywords: ["advice", "practical"], response: "Practical lesson: define risk before you define reward.", rel: { friendship: 1, romance: 0 }, end: true }
      ]
    }
  },
  work: {
    start: {
      response: "Work lane. Pick one: routine, standards, or limits.",
      next: [
        { keywords: ["routine"], response: "Routine keeps quality stable and prevents expensive surprises.", setFlag: "heard_work_routine", end: true },
        { keywords: ["standards"], response: "My standard is simple: no shortcuts that create future damage.", setFlag: "heard_work_standards", end: true },
        { keywords: ["limits", "boundary"], response: "My limits are clear: no coercion, no hidden terms.", setFlag: "heard_work_limits", end: true }
      ]
    }
  },
  rumors: {
    start: {
      response: "Rumor lane. Pick one: market, roads, or local tensions.",
      next: [
        { keywords: ["market"], response: "Market rumor: scarcity is being exaggerated by a few loud sellers.", setFlag: "heard_rumor_market", end: true },
        { keywords: ["roads", "route"], response: "Road rumor: two paths are safe in daylight, risky after dusk.", setFlag: "heard_rumor_roads", end: true },
        { keywords: ["local", "tension"], response: "Local rumor: people are polite, but buying like trouble is coming.", setFlag: "heard_rumor_local", end: true }
      ]
    }
  },
  trust: {
    start: {
      response: "Trust lane. Pick one: proof, boundaries, or loyalty.",
      next: [
        { keywords: ["proof"], response: "Proof is repeated honesty under inconvenience.", rel: { friendship: 1, romance: 0 }, setFlag: "trust_proof", end: true },
        { keywords: ["boundaries"], response: "Boundaries protect both trade and intimacy.", rel: { friendship: 1, romance: 0 }, setFlag: "trust_boundaries", end: true },
        { keywords: ["loyalty"], response: "Loyalty is expensive. I give it where it is reciprocated.", rel: { friendship: 1, romance: 1 }, setFlag: "trust_loyalty", end: true }
      ]
    }
  }
};

const TOPIC_TRIGGERS = {
  background: ["background", "past", "history", "origin"],
  work: ["work", "job", "occupation", "standards"],
  rumors: ["rumor", "gossip", "news", "whispers"],
  trust: ["trust", "loyalty", "proof", "boundaries"]
};

const TOPICS = {
  rumors: {
    keywords: ["rumor", "rumours", "news", "gossip"],
    response: "If you want depth, ask for topics and pick rumors."
  },
  work: {
    keywords: ["work", "job", "trade", "occupation"],
    response: "I take contracts worth the risk."
  },
  roads: {
    keywords: ["road", "roads", "route", "travel"],
    response: "Roads are passable, but don't travel predictable routes after dark."
  },
  weather: {
    keywords: ["weather", "rain", "storm", "fog", "snow"],
    response: "Weather decides who is cautious and who is careless."
  },
  help: {
    keywords: ["help", "assist", "aid"],
    response: "Ask clearly. If I can help, I will."
  }
};

const NPC_CONFIG = {
  ...npcSkeleton,
  name: "New Character",
  occupation: "traveler",
  description: "A new person stands here, watching the crowd with calm focus.",
  currentLocation: NPC_ROOM,
  greeting: "Hello. First time in this part of town?",
  peaceful: true,
  merchant: false,
  gender: "unknown",
  race: "human",
  age: 30,
  health: 70,
  maxHealth: 70,
  hostile: false,
  aggression: 0,
  collectable: false,
  relationshipStage: "stranger",
  friendshipLevel: 0,
  romanceLevel: 0,
  reputation: 0,
  dialogue: [
    "I've seen this square on good days and bad days. It always teaches you something.",
    "People reveal themselves when they think nobody is listening."
  ],
  topics: TOPICS,
  topicBranches: TOPIC_BRANCHES,
  topicTriggers: TOPIC_TRIGGERS,
  schedule: SCHEDULE,
  behaviors: BEHAVIORS,
  approachMessages: {
    stranger: "They give you a careful nod. 'New face. Keep your words clear and we'll get along.'",
    friend: "They relax their shoulders. 'Good to see you. What problem are we solving today?'",
    romantic_interest: "A small smile. 'Careful. You're making this conversation less professional.'"
  },
  timeDialogue: {
    morning: { greeting: "Morning. Best hour for clear decisions.", chat: "Mornings are for planning, not improvising." },
    evening: { greeting: "Evening. Pressure makes people sloppy. Stay sharp.", chat: "At dusk, priorities reveal themselves." }
  },
  personality: {
    traits: ["calm", "observant", "pragmatic"],
    primary: "observant",
    speechPattern: "measured",
    emotionalRange: ["neutral", "warm", "guarded"],
    defaultEmotion: "neutral"
  },
  memory: {
    giftsReceived: [],
    specialMoments: [],
    favoriteInteractions: []
  },
  emotionalState: "neutral",
  conversationTopics: [],
  insideJokes: [],
  inventory: [],
  stockRotation: {},
  prices: {},
  questHints: ["Ask for topics.", "Try: topics, then trust."],
  tags: ["npc", "template", "universal", "advanced_dialogue"]
};

const validateNpcConfig = (cfg) => {
  const issues = [];
  const behaviorIds = new Set(Object.keys(cfg.behaviors || {}));
  for (const slot of (cfg.schedule || [])) {
    for (const behaviorId of (slot.behaviors || [])) {
      if (!behaviorIds.has(behaviorId)) {
        issues.push(`schedule uses undefined behavior '${behaviorId}'`);
      }
    }
  }
  for (const [topicId, tree] of Object.entries(cfg.topicBranches || {})) {
    if (!tree.start || !tree.start.response || !Array.isArray(tree.start.next)) {
      issues.push(`topicBranches.${topicId} is missing a valid start node`);
    }
  }
  return issues;
};

const configIssues = validateNpcConfig(NPC_CONFIG);
if (configIssues.length > 0) {
  modAPI.displayOutput({
    success: false,
    message: "[MOD ERROR] NPC config validation failed:\n- " + configIssues.join("\n- ")
  });
  throw new Error("NPC template validation failed.");
}

modAPI.addNPCCharacter(NPC_ID, NPC_CONFIG, {
  placeInRoomEntitySlot: true
});

const buildStatusMessage = () => {
  const npc = modAPI.getEntities()[NPC_ID];
  if (!npc) return "NPC not found.";
  const room = npc.currentLocation || "(unknown)";
  const topics = npc.dialogueLibrary && npc.dialogueLibrary.wordPatterns
    ? Object.keys(npc.dialogueLibrary.wordPatterns).length
    : 0;
  const branches = npc.topicBranches ? Object.keys(npc.topicBranches).length : 0;
  return (
    `NPC: ${npc.name}\n` +
    `Room: ${room}\n` +
    `Merchant: ${npc.merchant ? "yes" : "no"}\n` +
    `Keyword Topics: ${topics}\n` +
    `Branch Topics: ${branches}\n` +
    `Traits: ${(npc.personality && npc.personality.traits) ? npc.personality.traits.join(", ") : "(none)"}`
  );
};

modAPI.addCommand("universal_npc_status", () => {
  return { success: true, message: buildStatusMessage() };
});

modAPI.addCommand("npcmodstatus", () => {
  return { success: true, message: buildStatusMessage() };
});

modAPI.addCommand("npcmodvalidate", () => {
  const issues = validateNpcConfig(NPC_CONFIG);
  if (!issues.length) return { success: true, message: "NPC template validation: OK." };
  return { success: false, message: "NPC template validation issues:\n- " + issues.join("\n- ") };
});

modAPI.onCommand((action, target) => {
  const raw = `${action || ""} ${target || ""}`.trim().toLowerCase();
  if (
    raw === "unknown universal_npc_status" ||
    raw === "unknown npcmodstatus" ||
    raw === "unknown npc status" ||
    raw === "unknown mod npc status"
  ) {
    return { handled: true, success: true, message: buildStatusMessage() };
  }
  return null;
}, { priority: 120 });

modAPI.displayOutput({
  success: true,
  message:
    "[MOD LOADED] Universal NPC Template (Advanced)\n" +
    `NPC ID: ${NPC_ID}\n` +
    `Room: ${NPC_ROOM}\n` +
    "Try: talk new character\n" +
    "Try: talk topics\n" +
    "Try: talk trust\n" +
    "Try: universal_npc_status\n" +
    "Try: npcmodvalidate"
});
