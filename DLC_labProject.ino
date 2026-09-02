#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// ================= RFID =================
#define SS_PIN   5
#define RST_PIN  22

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= BUTTONS =================
#define YES_BTN   32
#define NO_BTN    33
#define UNDO_BTN  25

// ================= CONTROL =================
#define SESSION   16

// ================= LEDS =================
#define AUTH_LED   13
#define TIME_LED    14
#define ENABLE_LED  2

#define DUP_LED     17
#define EVENT_LED   21

#define YES_LED     26
#define NO_LED      4
#define UNDO_LED    27

#define BUZZER      15

// ================= VOTER =================
struct Voter {
  String uid;
  bool voted;
  int vote;
  unsigned long firstSeen;
  unsigned long voteTime;
};

#define MAX_VOTERS 20
Voter voters[MAX_VOTERS];
int voterCount = 0;

int activeVoterIndex = -1;

// ================= STATE =================
bool sessionActive = false;
bool systemLocked = false;

int yesCount = 0;
int noCount = 0;

String question = "";
int sessionTime = 0;
unsigned long startTime = 0;

// ================= DECLARATIONS =================
int findVoter(String uid);
void handleCard(String uid);
bool setVote(int value);
void undoVote();
void duplicateAlert();
void resetVoteLEDs();
void setOnlyOneLED(int led);
void flash(int pin);
void beep(int f);
void waitRelease(int pin);
void showVote(int led, int freq);
void showResult();
void shutdownSystem();
void resetAll();

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(YES_BTN, INPUT_PULLUP);
  pinMode(NO_BTN, INPUT_PULLUP);
  pinMode(UNDO_BTN, INPUT_PULLUP);

  pinMode(SESSION, OUTPUT);

  pinMode(AUTH_LED, OUTPUT);
  pinMode(TIME_LED, OUTPUT);
  pinMode(ENABLE_LED, OUTPUT);

  pinMode(DUP_LED, OUTPUT);
  pinMode(EVENT_LED, OUTPUT);

  pinMode(YES_LED, OUTPUT);
  pinMode(NO_LED, OUTPUT);
  pinMode(UNDO_LED, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  resetAll();

  Serial.println("\n=== VOTING SYSTEM START ===");

  Serial.println("Enter Question:");
  while (question == "") {
    if (Serial.available()) {
      question = Serial.readStringUntil('\n');
      question.trim();
    }
  }

  Serial.println("Enter Session Time:");
  while (sessionTime == 0) {
    if (Serial.available()) {
      sessionTime = Serial.parseInt();
    }
  }

  sessionActive = true;
  startTime = millis();

  digitalWrite(SESSION, HIGH);
  digitalWrite(ENABLE_LED, HIGH);
  digitalWrite(TIME_LED, HIGH);

  Serial.println("SESSION STARTED");
}

// ================= LOOP =================
void loop() {

  if (sessionActive && millis() - startTime > sessionTime * 1000) {

    sessionActive = false;
    systemLocked = true;

    digitalWrite(SESSION, LOW);
    digitalWrite(ENABLE_LED, LOW);
    digitalWrite(TIME_LED, LOW);

    Serial.println("\n=== SESSION END ===");
    showResult();
  }

  // ================= RFID =================
  if (sessionActive && !systemLocked &&
      rfid.PICC_IsNewCardPresent() &&
      rfid.PICC_ReadCardSerial()) {

    String uid = "";

    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();
    handleCard(uid);

    rfid.PICC_HaltA();
  }

  // ================= YES =================
  if (digitalRead(YES_BTN) == LOW) {
    delay(50);
    if (setVote(1)) {
      yesCount++;
      setOnlyOneLED(YES_LED);
      showVote(YES_LED, 1000);
    }
    waitRelease(YES_BTN);
  }

  // ================= NO =================
  if (digitalRead(NO_BTN) == LOW) {
    delay(50);
    if (setVote(0)) {
      noCount++;
      setOnlyOneLED(NO_LED);
      showVote(NO_LED, 800);
    }
    waitRelease(NO_BTN);
  }

  // ================= UNDO =================
  if (digitalRead(UNDO_BTN) == LOW) {
    delay(50);
    if (activeVoterIndex != -1) {
      setOnlyOneLED(UNDO_LED);
      undoVote();
    }
    waitRelease(UNDO_BTN);
  }
}

// ================= RFID =================
void handleCard(String uid) {

  int index = findVoter(uid);

  if (index == -1) {

    if (voterCount < MAX_VOTERS) {
      voters[voterCount].uid = uid;
      voters[voterCount].voted = false;
      voters[voterCount].vote = -1;
      voters[voterCount].firstSeen = millis();
      voters[voterCount].voteTime = 0;
      voterCount++;
    }

    activeVoterIndex = findVoter(uid);

    resetVoteLEDs();

    flash(AUTH_LED);
    beep(1000);

    Serial.println("NEW CARD ✔");
    return;
  }

  activeVoterIndex = index;

  if (voters[index].voted) {
    Serial.println("DUPLICATE ❌");
    duplicateAlert();
  } else {
    flash(AUTH_LED);
    beep(1000);
  }
}

// ================= LED FIX =================
void setOnlyOneLED(int led) {

  digitalWrite(YES_LED, LOW);
  digitalWrite(NO_LED, LOW);
  digitalWrite(UNDO_LED, LOW);

  digitalWrite(led, HIGH);
}

// ================= VOTE =================
bool setVote(int value) {

  if (activeVoterIndex == -1) return false;

  Voter &v = voters[activeVoterIndex];

  if (v.voted) return false;

  v.voted = true;
  v.vote = value;
  v.voteTime = millis();

  return true;
}

// ================= UNDO =================
void undoVote() {

  Voter &v = voters[activeVoterIndex];

  if (v.voted) {

    if (v.vote == 1) yesCount--;
    if (v.vote == 0) noCount--;

    v.voted = false;
    v.vote = -1;

    flash(UNDO_LED);
    beep(1200);
  }
}

// ================= RESULT (ONLY UPDATED PART) =================
void showResult() {

  Serial.println("\n===== FINAL RESULT =====");

  Serial.print("QUESTION: ");
  Serial.println(question);

  Serial.print("YES COUNT: ");
  Serial.println(yesCount);

  Serial.print("NO COUNT: ");
  Serial.println(noCount);

  String winner = "DRAW";
  int winCount = 0;
  int loseCount = 0;

  if (yesCount > noCount) {
    winner = "YES WINS 🟢";
    winCount = yesCount;
    loseCount = noCount;
  } else if (noCount > yesCount) {
    winner = "NO WINS 🔴";
    winCount = noCount;
    loseCount = yesCount;
  }

  Serial.print("WINNER: ");
  Serial.println(winner);

  Serial.println("\nUID | VOTE | WIN_TIME(s) | LOSE_TIME(s) | DIFF(s)");

  for (int i = 0; i < voterCount; i++) {

    float t = 0;
    if (voters[i].voteTime > 0)
      t = (voters[i].voteTime - voters[i].firstSeen) / 1000.0;

    String voteType;

    if (voters[i].vote == 1) voteType = "YES";
    else if (voters[i].vote == 0) voteType = "NO";
    else voteType = "NONE";

    float winTime = 0;
    float loseTime = 0;

    if (winner.startsWith("YES") && voters[i].vote == 1) winTime = t;
    if (winner.startsWith("NO") && voters[i].vote == 0) winTime = t;

    if (winner.startsWith("YES") && voters[i].vote == 0) loseTime = t;
    if (winner.startsWith("NO") && voters[i].vote == 1) loseTime = t;

    float diff = winTime - loseTime;

    Serial.print(voters[i].uid);
    Serial.print(" | ");
    Serial.print(voteType);
    Serial.print(" | ");
    Serial.print(winTime);
    Serial.print(" | ");
    Serial.print(loseTime);
    Serial.print(" | ");
    Serial.println(diff);
  }

  Serial.println("\n👉 PRESS RESET BUTTON TO START NEW SESSION");

  shutdownSystem();
}

// ================= UTIL =================
void resetAll() { resetVoteLEDs(); }

void resetVoteLEDs() {
  digitalWrite(YES_LED, LOW);
  digitalWrite(NO_LED, LOW);
  digitalWrite(UNDO_LED, LOW);
  digitalWrite(EVENT_LED, LOW);
}

void shutdownSystem() {
  resetVoteLEDs();
  digitalWrite(AUTH_LED, LOW);
  digitalWrite(ENABLE_LED, LOW);
  digitalWrite(TIME_LED, LOW);
}

void flash(int pin) {
  digitalWrite(pin, HIGH);
  delay(200);
  digitalWrite(pin, LOW);
}

void beep(int f) {
  tone(BUZZER, f, 200);
}

void waitRelease(int pin) {
  while (digitalRead(pin) == LOW) delay(10);
}

void showVote(int led, int freq) {
  digitalWrite(EVENT_LED, HIGH);
  beep(freq);
  delay(300);
  digitalWrite(EVENT_LED, LOW);
}

void duplicateAlert() {
  digitalWrite(DUP_LED, HIGH);
  beep(1500);
  delay(500);
  digitalWrite(DUP_LED, LOW);
}

int findVoter(String uid) {
  for (int i = 0; i < voterCount; i++) {
    if (voters[i].uid == uid) return i;
  }
  return -1;
}