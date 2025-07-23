#include <Arduino.h>
#include "MIDIUSB.h"

volatile unsigned long contadorPulsos = 0;
unsigned long tiempoAnterior = 0;
unsigned long intervalo = 1000;  // medir cada 1000 ms
unsigned long frecuencia = 0;

int nota = 0;
int prevNota = 0;

unsigned long freqMin = 999999; // inicializa con valor alto
unsigned long freqMax = 0;

const int inputPin = 2; // pin con capacidad de interrupción

void contarPulso() {
  contadorPulsos++;
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, (uint8_t)(0x90 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, (uint8_t)(0x80 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void setup() {
  Serial.begin(31250);
  pinMode(inputPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(inputPin), contarPulso, RISING);
}
void loop() {
  unsigned long ahora = millis();

  if (ahora - tiempoAnterior >= intervalo) {
    noInterrupts();
    frecuencia = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    // Si no hay señal, apaga cualquier nota activa y no hagas nada más
    if (frecuencia == 0) {
      if (prevNota != 0) {
        noteOff(0, prevNota, 0);
        MidiUSB.flush();
        prevNota = 0;
      }
      Serial.println("Sin señal");
      tiempoAnterior = ahora;
      return;
    }

    // Actualizar mínimos y máximos
    if (frecuencia < freqMin) freqMin = frecuencia;
    if (frecuencia > freqMax) freqMax = frecuencia;

    // Evita errores si no hay suficiente rango
    if (freqMax - freqMin < 5) {
      nota = 0;
    } else {
      nota = map(frecuencia, freqMin, freqMax, 40, 90);
      nota = constrain(nota, 0, 127);
    }

    // Enviar nota solo si cambia
    if (nota != prevNota) {
      if (prevNota != 0) noteOff(0, prevNota, 0);
      if (nota != 0) noteOn(0, nota, 100);
      prevNota = nota;
    }

    MidiUSB.flush();

    // Serial.print("Frecuencia: ");
    // Serial.print(frecuencia);
    // Serial.print(" | Nota MIDI: ");
    // Serial.print(nota);
    // Serial.print(" | Mín: ");
    // Serial.print(freqMin);
    // Serial.print(" | Máx: ");
    // Serial.println(freqMax);

    tiempoAnterior = ahora;
  }
}
