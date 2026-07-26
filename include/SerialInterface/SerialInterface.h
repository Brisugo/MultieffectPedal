/*
 * SerialInterface.h
 * ------------------------------------------------------------
 * Interfaccia utente seriale per la gestione della EffectChain,
 * degli AudioEffect e dei relativi Param.
 *
 * Dipende dalle classi già esistenti nel progetto:
 *   EffectChain<MAX_EFFECTS>, AudioEffect, ParameterManager, Param, ParamType
 *
 * FORMATO COMANDI
 * ------------------------------------------------------------
 *  effect <id/nome>                           -> info + parametri abbreviati
 *  effect <id/nome> params                    -> dettaglio completo parametri
 *  effect <id/nome> params reset              -> reset di tutti i parametri
 *  effect <id/nome> param <nome> get          -> valore + info parametro
 *  effect <id/nome> param <nome> set <valore> -> imposta parametro
 *  effect <id/nome> param <nome> reset        -> reset singolo parametro
 *  effect <id/nome> enable|disable            -> abilita/disabilita effetto
 *
 *  chain                                      -> elenco catena effetti
 *  chain move <id> <nuova_pos>                -> sposta un effetto
 *  chain size                                 -> numero di effetti
 *
 *  compact [on|off]                           -> attiva/disattiva formato
 *                                                 ridotto (per Nextion)
 *  help                                       -> elenco comandi
 *
 * ABBREVIAZIONI
 * ------------------------------------------------------------
 * Ogni token puo' essere scritto per esteso, come prefisso
 * (es. "rev" per "reverb"), oppure come iniziali delle parole/
 * lettere maiuscole (es. "LP" per "LowPass", "cf" per "cutoff_freq").
 * Vale sia per i nomi dei comandi (effect/e, params/p, enable/en...)
 * sia per i nomi di effetti e parametri.
 *
 * FORMATO COMPATTO (Nextion)
 * ------------------------------------------------------------
 * Quando compactMode è attivo le risposte sono su singola riga,
 * con campi separati da '|' e liste separate da ',' (nome:valore):
 *   E|id|nome|enabled|p1:v1,p2:v2,...
 *   P|nome|tipo|valore|min|max|default          (singolo parametro)
 *   C|pos:id:nome:enabled,pos:id:nome:enabled,...
 *   OK|messaggio   /   ERR|messaggio
 * ------------------------------------------------------------
 */

#pragma once

#include <Arduino.h>
#include <ctype.h>
#include <stdlib.h>
#include "./Utils.h"

class SerialInterface
{
public:
    SerialInterface(EffectChain& effectChain, Stream& stream = Serial)
        : chain(effectChain), serial(stream), compactMode(false), lineLen(0) {}

    // Attiva/disattiva formato ridotto (utile per display Nextion)
    void setCompactMode(bool en) { compactMode = en; }
    bool isCompactMode() const   { return compactMode; }

    // Da chiamare ciclicamente nel loop(): legge e interpreta i comandi seriali
    void update() {
        while (serial.available()) {
            char c = (char)serial.read();
            if (c == '\r') continue;
            if (c == '\n') {
                lineBuf[lineLen] = '\0';
                if (lineLen > 0) processLine(lineBuf);
                lineLen = 0;
            } else if (lineLen < (sizeof(lineBuf) - 1)) {
                lineBuf[lineLen++] = c;
            }
        }
    }

private:
    static const uint8_t MAX_TOKENS  = 8;
    static const uint8_t LINE_BUFLEN = 96;

    EffectChain& chain;
    Stream& serial;
    bool compactMode;

    char lineBuf[LINE_BUFLEN];
    uint8_t lineLen;

    // ----------------------------------------------------------------
    // Parsing linea di comando
    // ----------------------------------------------------------------
    void processLine(char* line) {
        char* tokens[MAX_TOKENS];
        uint8_t count = 0;
        char* tok = strtok(line, " ");
        while (tok && count < MAX_TOKENS) {
            tokens[count++] = tok;
            tok = strtok(nullptr, " ");
        }
        if (count == 0) return;

        if (SerialInterfaceUtil::matchesAbbrev("effect", tokens[0])) {
            handleEffectCommand(tokens, count);
        } else if (SerialInterfaceUtil::matchesAbbrev("compact", tokens[0])) {
            handleCompactCommand(tokens, count);
        } else if (SerialInterfaceUtil::matchesAbbrev("help", tokens[0])) {
            handleHelp();
        } else {
            printError("comando sconosciuto (usa 'help')");
        }
    }

    // ----------------------------------------------------------------
    // effect <id/nome> ...
    // ----------------------------------------------------------------
    void handleEffectCommand(char** tokens, uint8_t count) {
        if (count == 1) { printChainList(); return; }

        const char* sub = tokens[1];

        if (SerialInterfaceUtil::matchesAbbrev("move", sub)) {
            if (count < 4) { printError("uso: effect move <id> <nuova_pos>"); return; }
            if (!SerialInterfaceUtil::isAllDigits(tokens[2]) || !SerialInterfaceUtil::isAllDigits(tokens[3])) {
                printError("id e posizione devono essere numeri");
                return;
            }
            uint8_t id  = (uint8_t)atoi(tokens[2]);
            uint8_t pos = (uint8_t)atoi(tokens[3]);
            bool ok = chain.moveEffect(id, pos);
            if (ok) printOk("effetto spostato");
            else printError("spostamento non riuscito");
            return;
        }

        if (count < 2) { printError("uso: effect <id/nome> ..."); return; }

        uint8_t id;
        AudioEffect* fx = resolveEffect(tokens[1], id);
        if (!fx) { printError("effetto non trovato"); return; }

        if (count == 2) {
            printEffectInfo(fx, id);
            return;
        }

        sub = tokens[2];

        // enable / disable
        if (SerialInterfaceUtil::matchesAbbrev("enable", sub)) {
            fx->setEnabled(true);
            printOk("effetto abilitato");
            return;
        }
        if (SerialInterfaceUtil::matchesAbbrev("disable", sub)) {
            fx->setEnabled(false);
            printOk("effetto disabilitato");
            return;
        }

        // params / param (entrambi iniziano per 'p', nessun'altra
        // sotto-parola comincia per 'p' -> nessuna ambiguità)
        if (sub[0] == 'p' || sub[0] == 'P') {
            handleParamCommand(fx, tokens, count);
            return;
        }

        printError("sottocomando non valido (params/param/enable/disable)");
    }

    // effect <id> params [reset]
    // effect <id> param <nome> [get|set <val>|reset]
    void handleParamCommand(AudioEffect* fx, char** tokens, uint8_t count) {
        ParameterManager& pm = fx->parameterManager;

        if (count == 3) {
            // effect <id> params
            printEffectParamsFull(fx);
            return;
        }

        if (count == 4) {
            if (SerialInterfaceUtil::matchesAbbrev("reset", tokens[3])) {
                // effect <id> params reset  -> reset di tutti i parametri
                pm.resetAll();
                fx->updateParams();
                printOk("parametri reimpostati");
                return;
            }
            // effect <id> param <nome>  -> get implicito
            Param* p = resolveParam(pm, tokens[3]);
            if (!p) { printError("parametro non trovato"); return; }
            printParamGet(p);
            return;
        }

        // count >= 5 -> effect <id> param <nome> <azione> [valore]
        Param* p = resolveParam(pm, tokens[3]);
        if (!p) { printError("parametro non trovato"); return; }

        const char* action = tokens[4];
        if (SerialInterfaceUtil::matchesAbbrev("get", action)) {
            printParamGet(p);
            return;
        }
        if (SerialInterfaceUtil::matchesAbbrev("reset", action)) {
            p->reset();
            fx->updateParams();
            printOk("parametro reimpostato");
            return;
        }
        if (SerialInterfaceUtil::matchesAbbrev("set", action)) {
            if (count < 6) { printError("uso: effect <id> param <nome> set <valore>"); return; }
            if (!p->isSettable()) { printError("parametro di sola lettura"); return; }
            bool ok = p->set(String(tokens[5]));
            if (ok) fx->updateParams();
            printParamSet(p, ok);
            return;
        }
        printError("azione non valida (get/set/reset)");
    }

    void handleCompactCommand(char** tokens, uint8_t count) {
        if (count >= 2) {
            if (SerialInterfaceUtil::matchesAbbrev("on", tokens[1])) compactMode = true;
            else if (SerialInterfaceUtil::matchesAbbrev("off", tokens[1])) compactMode = false;
            else { printError("uso: compact <on/off>"); return; }
        } else {
            compactMode = !compactMode;
        }
        printOk(compactMode ? "modalita compatta attiva" : "modalita normale attiva");
    }

    void handleHelp() {
        if (compactMode) {
            serial.println(F("H|effect,chain,compact,help"));
            return;
        }
        serial.println(F("Comandi disponibili:"));
        serial.println(F("  effect"));
        serial.println(F("  effect move <id> <nuova_pos>"));
        serial.println(F("  effect <id/nome>"));
        serial.println(F("  effect <id/nome> params"));
        serial.println(F("  effect <id/nome> params reset"));
        serial.println(F("  effect <id/nome> param <nome> get"));
        serial.println(F("  effect <id/nome> param <nome> set <valore>"));
        serial.println(F("  effect <id/nome> param <nome> reset"));
        serial.println(F("  effect <id/nome> enable|disable"));
        serial.println(F("  compact [on|off]"));
        serial.println(F("Nota: ogni parola puo' essere abbreviata (prefisso o iniziali)."));
    }

    // ----------------------------------------------------------------
    // Risoluzione effetto/parametro per id, nome esteso o abbreviato
    // ----------------------------------------------------------------
    AudioEffect* resolveEffect(const char* token, uint8_t& outId) {
        if (SerialInterfaceUtil::isAllDigits(token)) {
            uint8_t id = (uint8_t)atoi(token);
            AudioEffect* fx = chain.get(id);
            if (fx) { outId = id; return fx; }
        }
        for (uint8_t id = 0; id < chain.size(); id++) {
            AudioEffect* fx = chain.get(id);
            if (fx && SerialInterfaceUtil::matchesAbbrev(fx->getName(), token)) {
                outId = id;
                return fx;
            }
        }
        return nullptr;
    }

    Param* resolveParam(ParameterManager& pm, const char* token) {
        uint8_t n = pm.count();
        for (uint8_t i = 0; i < n; i++) {
            Param* p = pm.at(i);
            if (p && SerialInterfaceUtil::matchesAbbrev(p->name(), token)) return p;
        }
        return nullptr;
    }

    // ----------------------------------------------------------------
    // Stampe: info effetto
    // ----------------------------------------------------------------
    void printEffectInfo(AudioEffect* fx, uint8_t id) {
        ParameterManager& pm = fx->parameterManager;
        uint8_t n = pm.count();

        if (compactMode) {
            serial.print(F("E|"));
            serial.print(id); serial.print('|');
            serial.print(fx->getName()); serial.print('|');
            serial.print(fx->isEnabled() ? 1 : 0); serial.print('|');
            for (uint8_t i = 0; i < n; i++) {
                Param* p = pm.at(i);
                serial.print(p->name()); serial.print(':'); serial.print(p->get());
                if (i < n - 1) serial.print(',');
            }
            serial.println();
            return;
        }

        serial.print(F("Effetto [")); serial.print(id); serial.print(F("] "));
        serial.print(fx->getName());
        serial.println(fx->isEnabled() ? F(" - ON") : F(" - OFF"));
        // serial.println(fx->getInfo());
        serial.print(F("Parametri (")); serial.print(n); serial.println(F("):"));
        for (uint8_t i = 0; i < n; i++) {
            Param* p = pm.at(i);
            serial.print(F("  ")); serial.print(p->name());
            serial.print(F(" = ")); serial.println(p->get());
        }
        serial.println(F("(usa 'effect <id> params' per i dettagli completi)"));
    }

    void printEffectParamsFull(AudioEffect* fx) {
        ParameterManager& pm = fx->parameterManager;
        uint8_t n = pm.count();

        if (compactMode) {
            serial.print(F("P|"));
            for (uint8_t i = 0; i < n; i++) {
                Param* p = pm.at(i);
                serial.print(p->name()); serial.print(':');
                serial.print(paramTypeName(p->getType())); serial.print(':');
                serial.print(p->get()); serial.print(':');
                serial.print(p->minStr()); serial.print(':');
                serial.print(p->maxStr()); serial.print(':');
                serial.print(p->defaultStr());
                if (i < n - 1) serial.print(',');
            }
            serial.println();
            return;
        }

        serial.print(F("Parametri di ")); serial.println(fx->getName());
        for (uint8_t i = 0; i < n; i++) {
            Param* p = pm.at(i);
            serial.print(F("  ")); serial.print(p->name());
            serial.print(F(" [")); serial.print(paramTypeName(p->getType())); serial.print(F("]"));
            serial.print(F(" = ")); serial.print(p->get());
            serial.print(F("  (min=")); serial.print(p->minStr());
            serial.print(F(" max=")); serial.print(p->maxStr());
            serial.print(F(" default=")); serial.print(p->defaultStr());
            serial.println(F(")"));
            if (p->info() && p->info()[0] != '\0') {
                serial.print(F("      ")); serial.println(p->info());
            }
            if (!p->isSettable()) {
                serial.println(F("      (sola lettura)"));
            }
        }
    }

    void printParamGet(Param* p) {
        if (compactMode) {
            serial.print(F("P|"));
            serial.print(p->name()); serial.print('|');
            serial.print(paramTypeName(p->getType())); serial.print('|');
            serial.print(p->get()); serial.print('|');
            serial.print(p->minStr()); serial.print('|');
            serial.print(p->maxStr()); serial.print('|');
            serial.println(p->defaultStr());
            return;
        }
        serial.print(F("Parametro: ")); serial.println(p->name());
        serial.print(F("  Valore : ")); serial.println(p->get());
        serial.print(F("  Tipo   : ")); serial.println(paramTypeName(p->getType()));
        serial.print(F("  Min    : ")); serial.println(p->minStr());
        serial.print(F("  Max    : ")); serial.println(p->maxStr());
        serial.print(F("  Default: ")); serial.println(p->defaultStr());
        if (p->info() && p->info()[0] != '\0') {
            serial.print(F("  Info   : ")); serial.println(p->info());
        }
        serial.print(F("  Modificabile: "));
        serial.println(p->isSettable() ? F("si") : F("no"));
    }

    void printParamSet(Param* p, bool ok) {
        if (compactMode) {
            if (ok) { serial.print(F("OK|")); serial.print(p->name()); serial.print('|'); serial.println(p->get()); }
            else    { serial.print(F("ERR|")); serial.println(p->name()); }
            return;
        }
        if (ok) {
            serial.print(F("OK: ")); serial.print(p->name());
            serial.print(F(" impostato a ")); serial.println(p->get());
        } else {
            serial.print(F("Errore: valore non valido per ")); serial.print(p->name());
            serial.print(F(" (min=")); serial.print(p->minStr());
            serial.print(F(" max=")); serial.print(p->maxStr());
            serial.println(F(")"));
        }
    }

    // ----------------------------------------------------------------
    // Stampe: catena
    // ----------------------------------------------------------------
    void printChainList() {
        uint8_t n = chain.size();

        if (compactMode) {
            serial.print(F("C|"));
            for (uint8_t i = 0; i < n; i++) {
                AudioEffect* fx = chain.getOrder(i);
                if (!fx) continue;

                uint8_t id = fx->getId();
                
                serial.print(i); serial.print(':');
                serial.print(id); serial.print(':');
                serial.print(fx->getName()); serial.print(':');
                serial.print(fx->isEnabled() ? 1 : 0);
                if (i < n - 1) serial.print(',');
            }
            serial.println();
            return;
        }

        serial.println(F("Catena effetti:"));
        for (uint8_t i = 0; i < n; i++) {
            AudioEffect* fx = chain.getOrder(i);
            if (!fx) continue;

            uint8_t id = fx->getId();
            
            serial.print(F("  ")); serial.print(i); serial.print(F(". ["));
            serial.print(id); serial.print(F("] "));
            serial.print(fx->getName());
            serial.println(fx->isEnabled() ? F(" (ON)") : F(" (OFF)"));
        }
    }

    // ----------------------------------------------------------------
    // Messaggi generici
    // ----------------------------------------------------------------
    void printOk(const char* msg = nullptr) {
        if (compactMode) {
            serial.print(F("OK"));
            if (msg) { serial.print('|'); serial.print(msg); }
            serial.println();
        } else {
            serial.print(F("OK"));
            if (msg) { serial.print(F(": ")); serial.print(msg); }
            serial.println();
        }
    }

    void printError(const char* msg) {
        if (compactMode) {
            serial.print(F("ERR|")); serial.println(msg);
        } else {
            serial.print(F("Errore: ")); serial.println(msg);
        }
    }
};