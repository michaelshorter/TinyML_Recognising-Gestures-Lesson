# TinyML moves workshop

Teach an Arduino to recognise your movements, then watch a screen name them.

You collect motion data from an Arduino Nano 33 BLE Sense, train a small model in [Edge Impulse](https://edgeimpulse.com), run it on the board, and use a Processing sketch to show the recognised move as a word: `MOVE_1`, `MOVE_2`, `MOVE_3`, `IDLE` or `NOT_MOVES`.

The model runs on the board itself. The computer only displays what the board reports.

## What is in this repository

```
arduino/
  data_forwarder/     streams the motion sensor to Edge Impulse (for recording data)
  moves_inference/    runs your trained model on the board
processing/
  moves_display/      shows the recognised move as a word
```

## You will need

- An Arduino Nano 33 BLE Sense **Rev2** and a data-capable micro-USB cable
- Arduino IDE 2, with the **Arduino Mbed OS Nano Boards** package and the **Arduino_BMI270_BMM150** library
- Node.js and the Edge Impulse command line tools: `npm install -g edge-impulse-cli`
- A free Edge Impulse account
- Processing 4

Original Nano 33 BLE Sense (LSM9DS1)? Change the IMU include in both Arduino sketches to `#include <Arduino_LSM9DS1.h>`.

## The route at a glance

1. Put the data forwarder on the board
2. Record your data
3. Train and test the model
4. Build the library in Edge Impulse
5. Add it to the Arduino IDE
6. Set up and upload the inference sketch
7. Read the real confidence numbers
8. Switch to the Processing format
9. Run the Processing sketch

---

## 1. Put the data forwarder on the board

1. Plug the board in. In the Arduino IDE choose **Tools**, **Board**, **Arduino Mbed OS Nano Boards**, **Arduino Nano 33 BLE**, then pick the `usbmodem` port.
2. Open `arduino/data_forwarder/data_forwarder.ino` and click **Upload**. If it stalls, double-tap the board's reset button and try again.
3. Open the Serial Monitor at **115200**. You should see rows of six numbers that change when you move the board. Then **close the Serial Monitor**.

A fast-flashing light means the motion sensor did not start. Check your board version against the IMU library.

## 2. Record your data

1. In Edge Impulse, create a project.
2. In Terminal, run:
   ```
   edge-impulse-data-forwarder --clean
   ```
3. Log in, choose your project, and when it asks you to name the axes, type them in this order, with commas and no spaces:
   ```
   accX,accY,accZ,gyrX,gyrY,gyrZ
   ```
4. In **Data acquisition**, record each move under its own label: `MOVE_1`, `MOVE_2`, `MOVE_3`, plus `IDLE` (the board at rest) and `NOT_MOVES` (any other movement).

Tips:

- Record about a minute of one move with a still pause between each go, then use the split tool so Edge Impulse cuts out each movement.
- Do the moves the way you would really do them, at different speeds and with different people.
- Keep some data back for testing, from a separate session if you can.
- Use the same sketch for every recording in a project.

## 3. Train and test the model

1. In **Create impulse**, use a window size of 2000 ms. Set the frequency to the rate Studio shows for your recordings (about 72 Hz with this sketch). If Studio says that rate is not supported, pick the nearest one: Edge Impulse resamples your data to match.
2. Add a **Spectral analysis** processing block and a **Classification** learning block.
3. Optional: untick the three accelerometer axes so the model uses the gyroscope only. This often works better for gestures, and the inference sketch adapts automatically.
4. Generate features and check the feature explorer. Each move should form its own cluster.
5. Train, then read the confusion matrix and run **Model testing** on the test set.

---

## 4. Build the library in Edge Impulse

1. Open **Deployment** and choose **Arduino library**.
2. Leave the optimisation on the default (quantised) and click **Build**.
3. Download the `.zip`.

Rebuild and re-add the library every time you retrain.

## 5. Add it to the Arduino IDE

1. Choose **Sketch**, **Include Library**, **Add .ZIP Library**, and pick the file.
2. If you added an earlier build, the IDE may ask to replace it. Say yes.

## 6. Set up and upload the inference sketch

1. Open `arduino/moves_inference/moves_inference.ino`.
2. Change the first `#include` to your library's header name. It ends in `_inferencing.h` and is listed under **Sketch**, **Include Library**, **Contributed libraries**.
3. Set `DEBUG_ALL` to `true` for now, then click **Upload**. The first build takes a few minutes. If it stalls, double-tap the reset button and retry.

## 7. Read the real confidence numbers

1. Open the Serial Monitor at **115200**. You should see a line every quarter of a second, showing the winning class and the probability of every class.
2. Do each move about ten times, then idle, then casual movement.
3. For each class, choose a threshold above what it scores when it is wrong and below what it usually scores when it is right. Real confidences are often well below 0.80.

## 8. Switch to the Processing format

1. Set `DEBUG_ALL` to `false` and upload again.
2. **Close the Serial Monitor.** Only one program can use the port at a time.

## 9. Run the Processing sketch

1. Open `processing/moves_display/moves_display.pde`. Its folder must also be called `moves_display`.
2. In the **TWEAK THESE** block, set `CLASS_NAMES` to match your Edge Impulse labels, then `THRESHOLDS` and `HITS_NEEDED` from step 7.
3. Press **Play**.
4. The window should show **Heard:** changing. Hold a move and its word appears once the streak is long enough.

---

## Settings

**Processing sketch** (`moves_display.pde`), one column per class in each row:

| Setting | What it does |
|---|---|
| `CLASS_NAMES` | Must match your Edge Impulse labels. Capital letters do not matter. |
| `THRESHOLDS` | Confidence needed for a reading to count as a hit for that class. |
| `HITS_NEEDED` | Readings in a row needed before that class triggers. |
| `BACKGROUNDS` | Background colour for each word. |
| `ALLOWED_MISSES` | Stray readings that can interrupt a run without resetting it. |
| `MIN_GAP_MS` | Minimum time between two triggers. |
| `CLEAR_AFTER_MS` | 0 keeps a word until a different one triggers. Otherwise it clears after this many milliseconds. |
| `PORT_CONTAINS` | `"usbmodem"` on a Mac. Use `"COM"` on Windows. |

**Arduino sketch** (`moves_inference.ino`):

| Setting | What it does |
|---|---|
| `#include <..._inferencing.h>` | Your Edge Impulse library's header name. |
| `DEBUG_ALL` | `true` prints every class's probability. `false` prints `label,confidence` for Processing. |
| `INFER_EVERY_MS` | Time between answers. Hits needed times this number is how long you must hold a move. |

## If something is not right

| What you see | What to try |
|---|---|
| Fast-flashing light on the board | The motion sensor did not start. Check your board version and IMU library. |
| Upload stalls | Double-tap the reset button, pick the port again and retry. |
| Forwarder cannot detect data | Close the Serial Monitor, replug the board and check the data forwarder sketch is on it. |
| "No port found" in Processing | Check the board is plugged in and the Serial Monitor is closed. |
| **Heard** stays on a dash | `DEBUG_ALL` may still be `true`, or the board is not sending. |
| **Heard** changes but no word appears | A label does not match `CLASS_NAMES`, or a threshold or `HITS_NEEDED` is too high. |
| The wrong word keeps appearing | The model needs more varied data for those classes. Add data in Edge Impulse and retrain. |

## Good to know

- Thresholds are personal. They depend on your model and how you move, so set them again after any retrain.
- The sketches have not been tested against every Edge Impulse library version, so check your build compiles before the day.

## Licence

Add a licence of your choice before publishing. MIT is a common choice.
