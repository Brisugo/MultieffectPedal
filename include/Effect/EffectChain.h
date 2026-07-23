#pragma once

#include "Effects/Chorus.h"
#include "Effects/Delay.h"
#include "Effects/Distortion.h"
#include "Effects/Filter.h"
#include "Effects/Flanger.h"
#include "Effects/Fuzz.h"
#include "Effects/Phaser.h"
#include "Effects/Reverb.h"
#include "Effects/Limiter.h"

using namespace Params;

ChorusEffect        ChorusFx        ( "Chorus",        1,      Chorus::pm      );    // 6344     bytes
DelayEffect         DelayFx         ( "Delay",         2,      Delay::pm       );    // 8228     bytes
DistortionEffect    DistortionFx    ( "Distortion",    3,      Distortion::pm  );
FilterEffect        FilterFx        ( "Filter",        4,      Filter::pm      );
FlangerEffect       FlangerFx       ( "Flanger",       5,      Flanger::pm     );    // 5172     bytes
FuzzEffect          FuzzFx          ( "Fuzz",          6,      Fuzz::pm        );
PhaserEffect        PhaserFx        ( "Phaser",        7,      Phaser::pm      );    // 1084     bytes
LimiterEffect       LimiterFx       ( "Limiter",       8,      Limiter::pm     );
// ReverbEffect      ReverbFx        (Params::Reverb::pm);       // 9380     bytes
//                                               // Totale 30356    bytes

class EffectChain
{
public:
    void begin(){
        for (uint8_t i = 0; i < count; i++) {
            chain[i]->begin();
            chain[i]->setEnabled(false);
        }

        chain[count - 1]->setEnabled(true); // limiter
    }

    inline float process(float sample)
    {
        for (uint8_t i = 0; i < count; i++) {
            sample = chain[i]->process(sample);
        }
        return sample;
    }

    inline bool moveEffect(uint8_t id, uint8_t newPos)
    {
        if (id >= count || newPos >= count) return false;

        uint8_t curPos;
        AudioEffect* eff;
        for (uint8_t i = 0; i < count; i++) {
            if(chain[i]->getId() == id) { eff = chain[i]; curPos = i; break; }
        }
        
        if (curPos < newPos) {
            for (uint8_t i = curPos; i < newPos; i++) chain[i] = chain[i + 1];
        } else if (curPos > newPos) {
            for (uint8_t i = curPos; i > newPos; i--) chain[i] = chain[i - 1];
        }
        chain[newPos] = eff;

        return true;
    }

    AudioEffect* get(uint8_t id) {
        if (id >= count) return nullptr;

        for (uint8_t i = 0; i < count; i++) if(chain[i]->getId() == id) return chain[i];

        return nullptr;
    }

    AudioEffect* getOrder(uint8_t index) { return chain[index]; }

    uint8_t size() const { return count; }
private:
    AudioEffect* chain[8] = {
        &ChorusFx,
        &DelayFx,
        &DistortionFx,
        &FilterFx,
        &FlangerFx,
        &FuzzFx,
        &PhaserFx,
        &LimiterFx
    };

    uint8_t count = sizeof(chain) / sizeof(AudioEffect*);
};