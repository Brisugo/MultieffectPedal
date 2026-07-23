#include "Arduino.h"
#include <Audio.h>

#include "Effect/EffectChain.h"
#include "SerialInterface/SerialInterface.h"

Audio audio;
EffectChain effectChain;
SerialInterface serialEffects(effectChain);

float _p(float in){ return effectChain.process(in); }

void setup()
{    
    effectChain.begin();
    
    if ( !audio.begin(_p) ) while(1);
}

void loop() {
    serialEffects.update();
}