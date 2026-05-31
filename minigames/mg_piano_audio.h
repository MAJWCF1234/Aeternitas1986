#ifndef MG_PIANO_AUDIO_H
#define MG_PIANO_AUDIO_H

#define AET_PIANO_BASE_MIDI 48
#define AET_PIANO_NOTE_MS 210
#define AET_PIANO_GM_PROGRAM 0

void mg_piano_audio_init(void);
void mg_piano_audio_shutdown(void);

void mg_piano_play_key(char key_char);

void mg_piano_audio_enable(int on);
int mg_piano_audio_enabled(void);

const char *mg_piano_audio_backend_name(void);

#endif
