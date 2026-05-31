#include "mg_piano_audio.h"

#include <math.h>
#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

#define MIDI_CH 0

static signed char k_semi[128];

static void init_map(void) {
  static int done;
  if (done) return;
  {
    int i;
    for (i = 0; i < 128; i++) k_semi[i] = -1;
  }
  k_semi['1'] = 0;
  k_semi['!'] = 1;
  k_semi['2'] = 2;
  k_semi['@'] = 3;
  k_semi['3'] = 4;
  k_semi['4'] = 5;
  k_semi['$'] = 6;
  k_semi['5'] = 7;
  k_semi['%'] = 8;
  k_semi['6'] = 9;
  k_semi['^'] = 10;
  k_semi['7'] = 11;
  k_semi['8'] = 12;
  k_semi['*'] = 13;
  k_semi['9'] = 14;
  k_semi['('] = 15;
  k_semi['0'] = 16;
  k_semi['q'] = 17;
  k_semi['Q'] = 18;
  k_semi['w'] = 19;
  k_semi['W'] = 20;
  k_semi['e'] = 21;
  k_semi['E'] = 22;
  k_semi['r'] = 23;
  k_semi['t'] = 24;
  k_semi['T'] = 25;
  k_semi['y'] = 26;
  k_semi['Y'] = 27;
  k_semi['u'] = 28;
  k_semi['i'] = 29;
  k_semi['I'] = 30;
  k_semi['o'] = 31;
  k_semi['O'] = 32;
  k_semi['p'] = 33;
  k_semi['P'] = 34;
  k_semi['a'] = 35;
  k_semi['s'] = 36;
  k_semi['S'] = 37;
  k_semi['d'] = 38;
  k_semi['D'] = 39;
  k_semi['f'] = 40;
  k_semi['g'] = 41;
  k_semi['G'] = 42;
  k_semi['h'] = 43;
  k_semi['H'] = 44;
  k_semi['j'] = 45;
  k_semi['J'] = 46;
  k_semi['k'] = 47;
  k_semi['l'] = 48;
  k_semi['L'] = 49;
  k_semi['z'] = 50;
  k_semi['Z'] = 51;
  k_semi['x'] = 52;
  k_semi['c'] = 53;
  k_semi['C'] = 54;
  k_semi['v'] = 55;
  k_semi['V'] = 56;
  k_semi['b'] = 57;
  k_semi['B'] = 58;
  k_semi['n'] = 59;
  k_semi['m'] = 60;
  done = 1;
}

static int g_audio_on = 1;
static const char *g_backend = "silent";

#if defined(_WIN32)
static HMIDIOUT g_midi = NULL;

static DWORD midi_pack(unsigned status, unsigned d1, unsigned d2) {
  return (DWORD)status | ((DWORD)d1 << 8) | ((DWORD)d2 << 16);
}

static void midi_send_pc(unsigned program) {
  if (!g_midi) return;
  (void)midiOutShortMsg(g_midi, midi_pack(0xC0u | (unsigned)MIDI_CH, program, 0));
}

static void midi_send_note_on(int note, int velocity) {
  if (!g_midi) return;
  if (note < 0) note = 0;
  if (note > 127) note = 127;
  if (velocity < 1) velocity = 1;
  if (velocity > 127) velocity = 127;
  (void)midiOutShortMsg(g_midi,
                         midi_pack(0x90u | (unsigned)MIDI_CH, (unsigned)note,
                                   (unsigned)velocity));
}

static void midi_send_note_off(int note) {
  if (!g_midi) return;
  if (note < 0) note = 0;
  if (note > 127) note = 127;
  (void)midiOutShortMsg(g_midi,
                         midi_pack(0x80u | (unsigned)MIDI_CH, (unsigned)note, 0));
}

static DWORD WINAPI midi_note_off_thread(LPVOID param) {
  int note = (int)(intptr_t)param;
  Sleep((DWORD)AET_PIANO_NOTE_MS);
  midi_send_note_off(note);
  return 0;
}

static int midi_try_open(void) {
  MMRESULT r;
  if (g_midi) return 1;
  r = midiOutOpen(&g_midi, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
  if (r != MMSYSERR_NOERROR) {
    g_midi = NULL;
    return 0;
  }
  midi_send_pc(AET_PIANO_GM_PROGRAM);
  return 1;
}

static void midi_all_notes_off(void) {
  if (!g_midi) return;
  (void)midiOutShortMsg(g_midi, midi_pack(0xB0u | (unsigned)MIDI_CH, 123, 0));
}

static unsigned piano_hz_from_midi(int midi) {
  double f = 440.0 * pow(2.0, (midi - 69) / 12.0);
  if (f < 37.0) f = 37.0;
  if (f > 32767.0) f = 32767.0;
  return (unsigned)(f + 0.5);
}

static DWORD WINAPI beep_thread(LPVOID param) {
  unsigned hz = (unsigned)(uintptr_t)param;
  if (hz >= 37u && hz <= 32767u) Beep(hz, (DWORD)AET_PIANO_NOTE_MS);
  return 0;
}

static void play_midi_note(int midi_note) {
  const int velocity = 92;
  midi_send_note_on(midi_note, velocity);
  (void)CreateThread(NULL, 0, midi_note_off_thread,
                     (LPVOID)(intptr_t)midi_note, 0, NULL);
}

static void play_beep_note(int midi_note) {
  unsigned hz = piano_hz_from_midi(midi_note);
  (void)CreateThread(NULL, 0, beep_thread, (LPVOID)(uintptr_t)hz, 0, NULL);
}
#endif

void mg_piano_audio_enable(int on) { g_audio_on = on ? 1 : 0; }

int mg_piano_audio_enabled(void) { return g_audio_on; }

const char *mg_piano_audio_backend_name(void) { return g_backend; }

void mg_piano_audio_init(void) {
  g_backend = "silent";
#if defined(_WIN32)
  if (midi_try_open()) g_backend = "GM MIDI";
  else g_backend = "PC speaker";
#else
  g_backend = "silent";
#endif
}

void mg_piano_audio_shutdown(void) {
#if defined(_WIN32)
  if (g_midi) {
    midi_all_notes_off();
    (void)midiOutClose(g_midi);
    g_midi = NULL;
  }
#endif
}

void mg_piano_play_key(char key_char) {
  int semi;
  int midi;
  if (!g_audio_on) return;
  init_map();
  semi = k_semi[(unsigned char)key_char];
  if (semi < 0) return;
  midi = AET_PIANO_BASE_MIDI + semi;
  if (midi < 0) midi = 0;
  if (midi > 127) midi = 127;

#if defined(_WIN32)
  if (g_midi) {
    play_midi_note(midi);
    return;
  }
  play_beep_note(midi);
#else
  (void)midi;
#endif
}
