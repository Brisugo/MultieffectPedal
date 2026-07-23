// Audio/Audio.h
//
// Sostituisce la libreria Audio di Arduino R4 con un motore audio realtime
// su PC (basato su miniaudio). audio.begin(process) avvia un device audio
// che richiama la funzione "process" una volta per ogni campione, alla
// frequenza SAMPLE_RATE_HZ, esattamente come farebbe l'ISR sull'Arduino.
//
// La sorgente del segnale in ingresso e' scelta interattivamente all'avvio:
//   1) Microfono (live)
//   2) File audio (wav/mp3/flac...)
//   3) Segnale generato (oscillatore sinusoidale di test)
//
// L'uscita e' sempre riprodotta live sugli altoparlanti/uscita audio di default.
#pragma once

#include <cstdint>

class Audio {
public:
    Audio();
    ~Audio();

    // Avvia l'engine audio. "process" viene chiamata una volta per campione:
    // riceve il campione in ingresso (in [-1, +1]) e deve restituire il
    // campione da inviare in uscita (anch'esso atteso in [-1, +1]).
    // Ritorna false se l'inizializzazione del device audio fallisce.
    bool begin(float (*process)(float));

    // Ferma l'engine audio.
    void end();

private:
    void* _impl; // puntatore opaco all'implementazione (definita in Audio.cpp)
};
