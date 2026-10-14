# ODALID Porte monnaie - Qt (TP "Développement de l'interface graphique")

## Build with Qt Creator (Windows)
1. File > Open File or Project > `PorteMonnaie.pro`
2. Pick a **MinGW 64-bit** kit (the SDK library in `LIB/` is 64-bit MinGW). Qt 5.15 or Qt 6 both work.
3. Build & Run. `ODALID_Education.dll` is copied next to the exe automatically (`<build>/bin`).
   If it is not, copy `LIB/ODALID_Education.dll` there by hand.
4. Plug the reader (USB/CDC) *before* clicking **Connect**.

On Linux/macOS (or with `CONFIG+=sim`) the app runs on a **simulated card**, no hardware needed.

## Use (matches the TP screenshot)
Connect -> Selectionner la carte (poll + read identity + read counter) -> Mise à jour / Payer / Charger -> Disconnect.
Each transaction makes the reader flash the LED and beep (green = OK, red = error).

## Files
| File | Role |
|---|---|
| `mainwindow.*` | GUI and the 6 sequences of section 2.3 |
| `cardbackend.h` | Interface: one method per library action |
| `odalidbackend.*` | Real reader: ODALID SDK calls (OpenCOM, Version, RF_Power_Control, ISO14443_3_A_PollCard, Mf_Classic_*) |
| `simbackend.*` | In-memory fake card |
| `LIB/` | ODALID SDK headers, DLL and import lib (from Librairie_sample.7z) |

## Card map (section 2.2)
- Sector 2: block 8 "Identite", 9 Prenom, 10 Nom - read key A / write key B, key index 2
- Sector 3: block 12 "Porte Monnaie", 13 backup, 14 counter - read+decrement key A / write+increment key B, key index 3

Pay = `Mf_Classic_Decrement_Value(14 -> transfer to 13)` then `Mf_Classic_Restore_Value(13 -> 14)`.
Charge = same with `Mf_Classic_Increment_Value`.

## If something fails
- "Reader not found": reader not plugged / COM port busy (close any other ODALID tool).
- Authentication error on sector 2 or 3: the keys stored at index 2 / 3 of the reader do not match the card.
- Read_Value / Charge error on block 13/14: the blocks are not in MIFARE *value block* format.
  Initialise them once with `Mf_Classic_Write_Value(&reader, TRUE, 14, 12, AuthKeyB, 3)` and the same for block 13.
