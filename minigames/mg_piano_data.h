#ifndef MG_PIANO_DATA_H
#define MG_PIANO_DATA_H

int mg_piano_sheet_count(void);
const char *mg_piano_sheet_id(int i);
const char *mg_piano_sheet_title(int i);
const char *mg_piano_sheet_diff(int i);
const char *mg_piano_sheet_notation(int i);

#endif
