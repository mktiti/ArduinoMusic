#include <Tone.h>

#define RECORD_BUZZER_PIN 2
#define ECHO_BUZZER_PIN 3

#define STILL_FREQUENCY_PIN A0
#define BEEP_FREQUENCY_PIN A1

#define PLAYBACK_SPEED_PIN A2
#define BEEP_SPEED_PIN 9

#define PLAY_LED_PIN 8
#define RECORD_LED_PIN 9

#define MENU_BUTTON_PIN 10

#define MAX_NOTE_COUNT 20
#define SILENCE 0
#define PLAYING 1

struct Record {
    unsigned short preSilence;
    unsigned short beepSpeed; // 0 = continous
    unsigned short frequency;
    unsigned short length;
};

enum Mode { MENU, PLAYBACK, PLAYBACK_LOOP, VS_PLAYBACK, VS_PLAYBACK_LOOP, RECORDING };

struct PlaybackState {
  unsigned short currentElem;
  unsigned int currentStart;
};

struct RecordState {
  unsigned char currentNote;
  unsigned short prevFreq;
  unsigned short currentFreq;
};

#define MENU_ELEM_COUNT 5
enum MenuElem { PLAY, VS_PLAY, PLAY_LOOP, VS_PLAY_LOOP, RECORD };

struct MenuState {
  long unsigned int lastUpdate;
};

union State {
  PlaybackState playback;
  RecordState record;
  MenuState menu;
};

Mode mode = MENU;
State state;

struct Record records[MAX_NOTE_COUNT] {
  { 1000, 0, NOTE_E7, 83 },
  { 108, 0, NOTE_E7, 83 },
  { 200, 0, NOTE_E7, 83 },
  { 200, 0, NOTE_C7, 83 },
  { 108, 0, NOTE_E7, 83 },
  { 200, 0, NOTE_G7, 83 },
  { 700, 0, NOTE_G6, 83 },

  { 656, 0, NOTE_C7, 83 },
  { 490, 0, NOTE_G6, 83 },
  { 490, 0, NOTE_E6, 83 },
  { 490, 0, NOTE_A6, 83 },
  { 200, 0, NOTE_B6, 83 },
  { 200, 0, NOTE_AS6, 83 },
  { 108, 0, NOTE_A6, 83 },
};
char recordedCount = 14;

Tone recordTone;
Tone echoTone;

boolean isButtonPressed = false;

void setup() {
  Serial.begin(9600);

  recordTone.begin(RECORD_BUZZER_PIN);
  echoTone.begin(ECHO_BUZZER_PIN);

  pinMode(PLAY_LED_PIN, OUTPUT);
  pinMode(RECORD_LED_PIN, OUTPUT);

  pinMode(MENU_BUTTON_PIN, INPUT);

  loadMenu();
}

boolean shouldStep(unsigned int *timeDiff) {
  unsigned short toNext = (state.playback.currentElem % 2 == 1) ? records[state.playback.currentElem / 2].length
                                                                : records[state.playback.currentElem / 2].preSilence;
  if (*timeDiff >= toNext) {
    *timeDiff -= toNext;
    return true;
  }

  return false;
}

void playbackUpdate() {
  unsigned int timeDiff = millis() - state.playback.currentStart;

  while (shouldStep(&timeDiff)) {
    state.playback.currentElem++;
    if (state.playback.currentElem == 2 * recordedCount) {
      if (mode == PLAYBACK || mode == VS_PLAYBACK) { // Exit to menu
        loadMenu();
      } else { // Looping
        state.playback.currentElem = 0;
      }
    }
  }

  state.playback.currentStart = millis() - timeDiff;

  if (state.playback.currentElem % 2 == 1) {
    recordTone.play(records[state.playback.currentElem / 2].frequency);
  } else {
    recordTone.stop();
  }
}

MenuElem getMenuElem() {
  return static_cast<MenuElem>(map(analogRead(PLAYBACK_SPEED_PIN), 0, 1024, 0, MENU_ELEM_COUNT));
}

void onMenuButton() {
  if (mode == MENU) {
    MenuElem selectedElem = getMenuElem();
    switch (selectedElem) {
      case PLAY:
        mode = PLAYBACK;
        break;
      case VS_PLAY:
        mode = VS_PLAYBACK;
        break;
      case PLAY_LOOP:
        mode = PLAYBACK_LOOP;
        break;
      case VS_PLAY_LOOP:
        mode = VS_PLAYBACK_LOOP;
        break;
      case RECORD:
        mode = RECORDING;
        state.record = {
          .currentNote = 0,
          .prevFreq = 0,
          .currentFreq = 0
        };
        break;
    }

    if (selectedElem != RECORD) {
      state.playback = {
        .currentElem = 0,
        .currentStart = 0
      };
    }
  } else {
    loadMenu();
  }
}

void loadMenu() {
  mode = MENU;
  state.menu = { .lastUpdate = millis() };
  recordTone.stop();
}

char* menuNames[] = { "PLAY", "VS PLAY", "LOOP", "VS LOOP", "RECORD" };

void renderMenu() {
  Serial.print("Menu state: ");
  Serial.println(menuNames[getMenuElem()]);
}

void loop() {
  if (digitalRead(MENU_BUTTON_PIN)) {
    isButtonPressed = true;
  } else if (isButtonPressed) {
    isButtonPressed = false;
    onMenuButton();
  } else {
    switch (mode) {
      case MENU:
        renderMenu();
        break;
      default:
        Serial.print("Mode: ");
        Serial.println(menuNames[mode - 1]);
        break;
    }
  }

  delay(50);
}
