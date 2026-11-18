/*
  moves_display.pde
  Reads "label,confidence" lines from the Nano over USB serial and shows the
  word for the move it recognises:

      MOVE_1   MOVE_2   MOVE_3   IDLE   NOT_MOVES

  A word only appears after the same class has been heard enough times in a
  row, with high enough confidence. You set both numbers per class below.

  Requires
  - Processing 4
  - The Nano running moves_inference.ino with DEBUG_ALL = false
  - The Arduino Serial Monitor CLOSED (only one program can use the port)
*/

import processing.serial.*;

// =====================================================================
//  TWEAK THESE
//  Each row below has one column per class, in the SAME ORDER.
//  Column 0 = MOVE_1, 1 = MOVE_2, 2 = MOVE_3, 3 = IDLE, 4 = NOT_MOVES
// =====================================================================

// 1. CLASS NAMES
//    These must match the labels in Edge Impulse. Capital letters do not
//    matter (move_1 and MOVE_1 both work). If you named your moves
//    something else, change the words here. You can also add or remove
//    columns, as long as all four rows below keep the same length.
String[] CLASS_NAMES = { "MOVE_1", "MOVE_2", "MOVE_3", "IDLE", "NOT_MOVES" };

// 2. THRESHOLDS: how confident the model must be (0.0 to 1.0) before a
//    reading counts as a hit for that class.
//    Real confidences are often well below 0.80, so start around 0.50, then
//    set each one from your own readings (the ten takes method).
float[] THRESHOLDS   = {   0.50,     0.50,     0.50,    0.70,   0.70 };

// 3. HITS NEEDED: how many readings in a row must pass the threshold before
//    that class triggers. Bigger = more reliable but slower to respond.
//    Time to trigger is about HITS x INFER_EVERY_MS (250 ms in the Arduino
//    sketch), so 3 hits is roughly 0.75 seconds of holding the move.
int[]   HITS_NEEDED  = {   3,        3,        3,       3,      3    };

// 4. WORD COLOURS (optional): background colour for each class.
color[] BACKGROUNDS  = { #F2A93B, #6FC2B9, #E98FA3, #9DB4E0, #C9C2B0 };

// 5. OTHER SETTINGS
int     ALLOWED_MISSES = 1;      // stray readings that can interrupt a run without resetting it
int     MIN_GAP_MS     = 500;    // minimum time between two triggers
int     CLEAR_AFTER_MS = 0;      // 0 = a word stays until a different one triggers.
                                 // Otherwise the word clears after this many ms.
String  PORT_CONTAINS  = "usbmodem";  // Mac. On Windows, use "COM".

// =====================================================================
//  You should not need to change anything below this line
// =====================================================================

Serial port;

String currentLabel = "-";   // latest label heard (any class)
float  currentConf  = 0;     // and its confidence
int    lastIdx      = -1;    // class the current run belongs to
int    streak       = 0;     // hits in a row for that class
int    misses       = 0;     // stray readings since the last hit
int    lastTrigger  = -100000;
int    shownIdx     = -1;    // class whose word is on screen (-1 = none)
int    shownAt      = 0;
int[]  triggerCounts = new int[CLASS_NAMES.length];

void setup() {
  size(900, 520);
  textFont(createFont("Helvetica", 24));

  String[] ports = Serial.list();
  printArray(ports);

  String chosen = null;
  for (String p : ports) if (p.contains(PORT_CONTAINS)) chosen = p;
  if (chosen == null) {
    println("No port containing \"" + PORT_CONTAINS + "\" found. Is the board plugged in, and is the Serial Monitor closed?");
    exit();
    return;
  }

  port = new Serial(this, chosen, 115200);
  port.bufferUntil('\n');
}

int indexOfClass(String label) {
  for (int i = 0; i < CLASS_NAMES.length; i++) {
    if (CLASS_NAMES[i].toUpperCase().equals(label)) return i;
  }
  return -1;   // label is not in CLASS_NAMES, so it can never trigger
}

void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  String[] parts = split(trim(line), ',');
  if (parts.length != 2) return;          // ignore anything that is not "label,confidence"

  currentLabel = parts[0].trim().toUpperCase();
  currentConf  = float(parts[1]);

  int idx = indexOfClass(currentLabel);
  boolean isHit = (idx >= 0) && (currentConf >= THRESHOLDS[idx]);

  if (isHit) {
    misses = 0;
    if (idx == lastIdx) streak++;
    else { streak = 1; lastIdx = idx; }

    if (streak >= HITS_NEEDED[idx] && millis() - lastTrigger > MIN_GAP_MS) {
      shownIdx = idx;                      // show this class's word
      shownAt = millis();
      lastTrigger = millis();
      triggerCounts[idx]++;
      streak = 0;                          // holding a move fires again every HITS_NEEDED readings
    }
  } else {
    misses++;
    if (misses > ALLOWED_MISSES) {         // too many stray readings: start the run again
      streak = 0;
      lastIdx = -1;
    }
  }
}

void draw() {
  if (CLEAR_AFTER_MS > 0 && shownIdx >= 0 && millis() - shownAt > CLEAR_AFTER_MS) {
    shownIdx = -1;
  }

  background(shownIdx >= 0 ? BACKGROUNDS[shownIdx] : color(30));
  fill(shownIdx >= 0 ? 20 : 220);

  // the word for the recognised class
  textAlign(CENTER, CENTER);
  textSize(110);
  text(shownIdx >= 0 ? CLASS_NAMES[shownIdx] : "-", width / 2, height / 2 - 60);

  // live details, so you can see what the board is saying
  textAlign(LEFT, BASELINE);
  textSize(24);
  text("Heard: " + currentLabel + "  (" + nf(currentConf, 1, 2) + ")", 30, height - 130);
  int need = (lastIdx >= 0) ? HITS_NEEDED[lastIdx] : 0;
  text("Streak: " + streak + " / " + need, 30, height - 95);

  String counts = "Triggers:  ";
  for (int i = 0; i < CLASS_NAMES.length; i++) {
    counts += CLASS_NAMES[i] + " " + triggerCounts[i] + "   ";
  }
  text(counts, 30, height - 55);
}
