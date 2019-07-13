#include <Arduino.h>

#define BUZZER_PIN              8 // Shared with green led

char tone_notes[5][5] = {
  " ",
  "cCA ",
  "abcd",
  "AdAd",
  "f A "
  }; // a space represents a rest
int tone_beats[5][4] = {
  { 0 },
  { 4,6,10,8 },
  { 4,4,4,4 },
  { 8,4,8,4 },
  { 8,4,8,4 }
};
int tone_tempo = 80;

byte tone_matrix_size = (sizeof(tone_notes) / sizeof(tone_notes[0])) - 1;

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(tone);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C', 'A' }; 
  int tones[] = { 2110, 1880, 1675, 1581, 1409, 1255, 1118, 1055, 1184 };
  for (int i = 0; i < sizeof(names); i++) {
    if (names[i] == note) {
      //playTone(tones[i], duration);
      tone(BUZZER_PIN, tones[i], duration);
      delay(duration * 1.2);
      noTone(BUZZER_PIN);
    }
  }
}


void tone_player(int theme, byte repetitions = 1) {
  // put your main code here, to run repeatedly:
  int tone_notes_size = sizeof(tone_notes[theme]);

  for (int j = 0; j < repetitions; j++) {
    for (int i = 0; i < tone_notes_size; i++) {
      if (tone_notes[theme][i] == ' ') {
        delay(tone_beats[theme][i] * tone_tempo); // rest
      } else {
        playNote(tone_notes[theme][i], tone_beats[theme][i] * tone_tempo);
      }
    }
  }
}
