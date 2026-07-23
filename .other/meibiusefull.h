#pragma once
#include "Arduino.h"
#include "AudioEffects/Effects.h"
#include "EffectChain.h"

// ============================================================================
// Configurazione runtime della catena di effetti via porta seriale (USB CDC,
// 115200 baud). Permette di:
//   - elencare gli effetti disponibili e il loro stato        (list)
//   - abilitare / disabilitare un effetto                     (enable / disable <id>)
//   - spostare un effetto in una nuova posizione nella catena  (move <id> <pos>)
//   - modificare i parametri di un effetto                    (set <id> <param> <valore>)
//   - vedere i parametri validi per un effetto                (params <id>)
//
// IMPORTANTE: questo layer SOSTITUISCE l'uso di EffectChain<N> nell'ISR
// audio. EffectChain (come da .ino originale) elabora gli effetti solo
// nell'ordine in cui sono stati aggiunti con addEffect(), senza un modo
// per riordinarli o rimuoverli a runtime. Qui invece ogni effetto ha un ID
// FISSO assegnato in setup() (via registerEffect), mentre l'ordine di
// elaborazione e' un array separato (`order[]`) che i comandi seriali
// possono modificare in qualunque momento, anche a chain gia' avviata.
//
// Disabilitare un effetto NON lo toglie da order[]: ogni classe effetto
// gestisce gia' da sola il proprio stato (vedi "if (!enabled) return in;"
// in Effects.h), quindi un effetto disabilitato semplicemente fa da
// passthrough nella posizione in cui si trova. Questo evita di dover
// spostare puntatori quando abiliti/disabiliti, serve solo per move.
//
// Comandi seriali disponibili (apri il Monitor Seriale a 115200 baud, invio
//   di riga con newline):
//     help                        - mostra l'elenco comandi
//     list                        - mostra la catena corrente (ordine, id, stato)
//     params <id>                 - elenca i parametri validi per l'effetto
//     enable <id>  / disable <id> - abilita / disabilita un effetto
//     move <id> <pos>             - sposta l'effetto in posizione <pos> (1-based)
//     set <id> <param> <valore>   - imposta un parametro
//     set <id> voice <0|1> <hz>   - (solo Chorus) rate della voce 0 o 1

//   Esempio di sessione:
//     list
//       pos 1  id 0  [off] Distortion
//       pos 2  id 1  [off] Fuzz
//       pos 3  id 2  [ON ] Flanger
//       pos 4  id 3  [ON ] Phaser
//       pos 5  id 4  [off] Filter
//       pos 6  id 5  [off] Chorus
//       pos 7  id 6  [off] Delay
//       pos 8  id 7  [off] Reverb
//       pos 9  id 8  [ON ] Limiter
//     set 3 rate 0.8
//     set 3 mix 0.7
//     move 8 1          -> sposta il Delay (id 7) in prima posizione... attenzione: id 7 e' Delay, questo e' solo un esempio di sintassi
//     enable 4
//     disable 2
// ============================================================================

template <uint8_t MAX_EFFECTS>
class SerialEffects
{
public:
    SerialEffects(EffectChain<MAX_EFFECTS> *chain): chain(chain) {}

    // ---------------------------------------------------------------------------
    // Stampa la catena corrente in ordine di elaborazione.
    // ---------------------------------------------------------------------------
    inline void printList()
    {
        Serial.println(F("== Catena effetti =="));
        Serial.print("Size: ");
        Serial.println(chain->size());
        for (uint8_t i = 0; i < chain->size(); i++) {
            uint8_t id = chain->getOrder(i);
            AudioEffect *effect = chain->get(id);

            Serial.print(F("  ("));
            Serial.print(id);
            Serial.print(F(") "));
            Serial.print(effect->isEnabled() ? F("[ON ] ") : F("[off] "));
            Serial.println(effect->getName());
        }
    }

    inline void printParamsFor(AudioEffect *t, uint8_t id)
    {
        Serial.print(F("== "));
        Serial.print(t->getName());
        Serial.print(F(" (id "));
        Serial.print(id);
        Serial.println(F(") "));

        ParameterManager &pm = t->parameterManager;
        for (uint8_t i = 0; i < pm.count(); i++) {
            Param *p = pm.at(i);

            Serial.print(F("  "));
            Serial.print(p->name());
            Serial.print(F(" = "));

            Serial.print(p->get());
            Serial.print(F("  ("));
            Serial.print(p->minStr());
            Serial.print(F("-"));
            Serial.print(p->maxStr());
            Serial.println(F(")"));
        }
    }

    inline void printParam(Param *p){
        // Serial.print(F("  "));
        Serial.print(p->name());
        Serial.println(F(":"));

        Serial.print(F("  value: "));
        Serial.println(p->get());
        Serial.print(F("  settable: "));
        Serial.println(p->isSettable() ? "true" : "false");
        Serial.print(F("  min: "));
        Serial.println(p->minStr());
        Serial.print(F("  max: "));
        Serial.println(p->maxStr());
        Serial.print(F("  default: "));
        Serial.println(p->defaultStr());
        Serial.print(F("  info:\n  "));
        Serial.println(p->info());
    }

    inline void printHelp()
    {
        Serial.println(F("Comandi disponibili:"));
        Serial.println(F("  list                        - mostra la catena corrente"));
        Serial.println(F("  get <id> <param>            - mostra info del parametro"));
        Serial.println(F("  params <id>                 - elenca i nomi dei parametri validi"));
        Serial.println(F("  enable <id>                 - abilita l'effetto"));
        Serial.println(F("  disable <id>                - disabilita l'effetto"));
        Serial.println(F("  move <id> <pos>             - sposta l'effetto in posizione <pos> (1-based)"));
        Serial.println(F("  set <id> <param> <valore>   - imposta un parametro"));
        Serial.println(F("  help                        - questo messaggio"));
    }

    // ---------------------------------------------------------------------------
    // Parsing di una riga di comando (gia' null-terminated, senza \r\n).
    // ---------------------------------------------------------------------------
    inline void processLine(char* line)
    {
        char* cmd = strtok(line, " ");
        if (!cmd) return;

        if (!strcmp(cmd, "help")) { printHelp(); return; }
        if (!strcmp(cmd, "list")) { printList(); return; }

        if (!strcmp(cmd, "get")) {
            char* idStr = strtok(NULL, " ");
            char* param = strtok(NULL, " ");

            if (!idStr || !param) { Serial.println(F("uso: get <id> <param>")); return; }

            uint8_t id = (uint8_t)atoi(idStr);

            if (id >= chain->size()) { Serial.println(F("id non valido")); return; }

            printParam(chain->get(id)->parameterManager.find(param));
            
            return;
        }

        if (!strcmp(cmd, "params")) {
            char* idStr = strtok(NULL, " ");

            if (!idStr) { Serial.println(F("uso: params <id>")); return; }

            uint8_t id = (uint8_t)atoi(idStr);

            if (id >= chain->size()) { Serial.println(F("id non valido")); return; }

            printParamsFor(chain->get(id), id);

            return;
        }

        if (!strcmp(cmd, "enable") || !strcmp(cmd, "disable")) {
            char* idStr = strtok(NULL, " ");
            if (!idStr) { Serial.println(F("uso: enable/disable <id>")); return; }
            uint8_t id = (uint8_t)atoi(idStr);
            if (id >= chain->size()) { Serial.println(F("id non valido")); return; }
            bool en = !strcmp(cmd, "enable");
            chain->setEffectEnabled(id, en);
            Serial.print(chain->get(id)->getName());
            Serial.println(en ? F(": abilitato") : F(": disabilitato"));
            return;
        }

        if (!strcmp(cmd, "move")) {
            char* idStr  = strtok(NULL, " ");
            char* posStr = strtok(NULL, " ");
            if (!idStr || !posStr) { Serial.println(F("uso: move <id> <posizione 1-based>")); return; }
            uint8_t id = (uint8_t)atoi(idStr);
            int pos1 = atoi(posStr);
            if (id >= chain->size() || pos1 < 1 || pos1 > chain->size()) {
                Serial.println(F("parametri non validi"));
                return;
            }
            chain->moveEffect(id, (uint8_t)(pos1 - 1));
            Serial.println(F("catena aggiornata:"));
            printList();
            return;
        }

        if (!strcmp(cmd, "set")) {
            char* idStr = strtok(NULL, " ");
            char* param = strtok(NULL, " ");
            char* rest  = strtok(NULL, "");   // tutto cio' che resta nella riga (puo' contenere piu' token)
            
            if (!idStr || !param || !rest) { Serial.println(F("uso: set <id> <param> <valore>")); return; }
            
            uint8_t id = (uint8_t)atoi(idStr);
            
            if (id >= chain->size()) { Serial.println(F("id non valido")); return; }
            
            AudioEffect *eff = chain->get(id);
            bool ret = eff->parameterManager.setByName(param, rest);
            eff->updateParams();
            
            Serial.println(ret ? F("ok") : F("parametro sconosciuto (usa 'params <id>' per l'elenco)"));
            
            return;
        }

        Serial.println(F("comando sconosciuto (usa 'help')"));
    }

    // ---------------------------------------------------------------------------
    // Buffer di ricezione riga + polling non bloccante, da chiamare in loop().
    // ---------------------------------------------------------------------------

    inline void serialConfigBegin(uint32_t baud = 115200)
    {
        Serial.begin(baud);
        Serial.println(F("Catena di effetti pronta. Digita 'help' per i comandi."));
        printList();
    }

    inline void serialConfigPoll()
    {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\r') continue;
            if (c == '\n') {
                lineBuf[lineLen] = '\0';
                if (lineLen > 0) processLine(lineBuf);
                lineLen = 0;
            } else if (lineLen < sizeof(lineBuf) - 1) {
                lineBuf[lineLen++] = c;
            }
        }
    }

private:
    char lineBuf[64];
    uint8_t lineLen = 0;

    EffectChain<MAX_EFFECTS> *chain;
};