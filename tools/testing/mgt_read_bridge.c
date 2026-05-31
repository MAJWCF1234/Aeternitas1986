#include "mgt_read.h"
#include "mgt_game_sim.h"

#include <stdio.h>
#include <string.h>

static MgtReadDocument g_doc;
static int g_has_doc;
static MgtReadResolveFn g_resolver;

typedef struct {
  const char *id;
  const char *title;
  const char *body;
} HarnessReadEntry;

static const HarnessReadEntry k_harness_reads[] = {
    {"leaflet", "Orientation Leaflet",
     "The leaflet reads: \"WELCOME TO AETERNITAS. TAKE ONLY WHAT YOU CAN "
     "CARRY. TRUST DOORS LAST.\" A faded line at the bottom says \"Printed for "
     "the 1986 orientation disk.\""},
    {"engineering_tome", "Engineering Tome",
     "The tome teaches a practical doctrine: listen for stress, respect "
     "leverage, and never trust a quiet gear that should be singing. Each "
     "chapter names a failure mode before it names a fix."},
    {"merchant_ledger", "Merchant Ledger",
     "Columns of prices, favors, and old debts. In the margins: \"Never "
     "discount in fog. Never extend credit to a masked poet. Always know the "
     "exits.\""},
    {"lore_scroll", "Lore Scroll",
     "The scroll recounts waystones, rifts, and the old argument that Hollow "
     "Ridge was never meant to be a border — only a seam."},
    {"herbalism_guide", "Herbalism Guide",
     "The guide warns that every useful plant has at least one bad lookalike. "
     "Smell, stem, season, then cut."},
    {"linguistics_textbook", "Linguistics Textbook",
     "The first lesson is not vocabulary but posture: every language protects "
     "what its speakers fear losing."},
    {"crafting_manual", "Crafting Manual",
     "Most pages are obvious; a few margins suggest combinations without naming "
     "them. You close it no wiser, only curious."},
    {"hollow_ridge_primer", "Hollow Ridge Primer",
     "Hollow Ridge Primer — excerpt:\n\n"
     "Veritasfurtum frays where divine attention thins. Waystones mark safe "
     "crossings; rifts are arguments between layers that no longer agree."},
    {"ancient_tome", "Ancient Tome",
     "The tome speaks of the Architect's lattice, waystones, and rifts where "
     "borrowed worlds refuse to leave."},
    {"book", "Old Books",
     "The pages discuss artifacts, thresholds, and the danger of mistaking a "
     "locked door for a wall. Several notes mention the Architect and Elysium "
     "crystals."},
};

static int id_is_readable(const char *id) {
  MgtReadKind k;
  if (!id || !id[0]) return 0;
  k = mgt_read_classify_source_id(id);
  if (k != MGT_READ_NOTE) return 1;
  if (strstr(id, "paper") || strstr(id, "primer") || strstr(id, "pamphlet"))
    return 1;
  return 0;
}

static int harness_builtin_resolve(const char *source_id, MgtReadDocument *doc) {
  size_t i;
  if (!source_id || !doc) return 0;
  for (i = 0; i < sizeof k_harness_reads / sizeof k_harness_reads[0]; i++) {
    if (strcmp(source_id, k_harness_reads[i].id) != 0) continue;
    memset(doc, 0, sizeof *doc);
    doc->valid = 1;
    doc->kind = mgt_read_classify_source_id(source_id);
    snprintf(doc->title, sizeof doc->title, "%s", k_harness_reads[i].title);
    snprintf(doc->source_id, sizeof doc->source_id, "%s", source_id);
    snprintf(doc->body, sizeof doc->body, "%s", k_harness_reads[i].body);
    return 1;
  }
  return 0;
}

void mgt_read_register_resolver(MgtReadResolveFn fn) { g_resolver = fn; }

int mgt_read_resolve_item(const char *source_id, MgtReadDocument *doc) {
  if (!source_id || !doc) return 0;
  if (g_resolver && g_resolver(source_id, doc)) return 1;
  return harness_builtin_resolve(source_id, doc);
}

int mgt_read_harness_fill_from_sim(const MgtGameSim *sim) {
  MgtReadDocument doc;
  int i;
  if (!sim) return 0;
  for (i = 0; i < sim->inv_n; i++) {
    if (!id_is_readable(sim->inv[i])) continue;
    if (!mgt_read_resolve_item(sim->inv[i], &doc)) continue;
    mgt_read_set_document(&doc);
    return 1;
  }
  return 0;
}

const char *mgt_read_kind_label(MgtReadKind kind) {
  switch (kind) {
  case MGT_READ_BOOK:
    return "BOOK";
  case MGT_READ_SIGN:
    return "SIGN";
  case MGT_READ_SCROLL:
    return "SCROLL";
  case MGT_READ_LEDGER:
    return "LEDGER";
  case MGT_READ_LETTER:
    return "LETTER";
  case MGT_READ_MAP:
    return "MAP";
  case MGT_READ_MUSIC:
    return "MUSIC";
  case MGT_READ_NOTE:
  default:
    return "NOTE";
  }
}

MgtReadKind mgt_read_classify_source_id(const char *source_id) {
  char norm[64];
  size_t i, n;
  if (!source_id || !source_id[0]) return MGT_READ_NOTE;
  for (i = 0, n = 0; source_id[i] && n + 1 < sizeof norm; i++) {
    char c = source_id[i];
    if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
    norm[n++] = c;
  }
  norm[n] = '\0';
  if (strstr(norm, "book") || strstr(norm, "tome") || strstr(norm, "manual") ||
      strstr(norm, "guide") || strstr(norm, "handbook") || strstr(norm, "journal") ||
      strstr(norm, "logbook") || strstr(norm, "primer") || strstr(norm, "textbook"))
    return MGT_READ_BOOK;
  if (strstr(norm, "ledger")) return MGT_READ_LEDGER;
  if (strstr(norm, "sheet") || strstr(norm, "music") || strstr(norm, "reel"))
    return MGT_READ_MUSIC;
  if (strstr(norm, "scroll") || strstr(norm, "parchment")) return MGT_READ_SCROLL;
  if (strstr(norm, "letter") || strstr(norm, "envelope") || strstr(norm, "leaflet") ||
      strstr(norm, "note") || strstr(norm, "missive"))
    return MGT_READ_LETTER;
  if (strstr(norm, "sign") || strstr(norm, "plaque") || strstr(norm, "notice") ||
      strstr(norm, "waystone") || strstr(norm, "marker") || strstr(norm, "inscription"))
    return MGT_READ_SIGN;
  if (strstr(norm, "map") || strstr(norm, "chart")) return MGT_READ_MAP;
  return MGT_READ_NOTE;
}

void mgt_read_set_document(const MgtReadDocument *doc) {
  if (!doc || !doc->valid) {
    g_has_doc = 0;
    memset(&g_doc, 0, sizeof g_doc);
    return;
  }
  g_doc = *doc;
  g_doc.valid = 1;
  g_has_doc = 1;
}

const MgtReadDocument *mgt_read_current_document(void) {
  return g_has_doc ? &g_doc : NULL;
}

void mgt_read_clear_document(void) {
  g_has_doc = 0;
  memset(&g_doc, 0, sizeof g_doc);
}

int mgt_read_take_document(MgtReadDocument *out) {
  if (!out) return 0;
  if (!g_has_doc) {
    memset(out, 0, sizeof *out);
    return 0;
  }
  *out = g_doc;
  return 1;
}
