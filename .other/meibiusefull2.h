/*
 * SerialInterface.h
 * ------------------------------------------------------------------------
 * Interfaccia seriale testuale per pilotare una EffectChain (catena di
 * AudioEffect / Param) da un terminale seriale o da uno schermo intelligente
 * (es. Nextion in modalita' "passthrough" testuale via UART).
 *
 * IMPORTANTE:
 *   Includi PRIMA di questo file gli header con le definizioni complete di
 *   EffectChain, AudioEffect, ParameterManager, Param, ParamType.
 *   Questo file non li include per non assumere nomi di file specifici del
 *   tuo progetto.
 *
 * ------------------------------------------------------------------------
 * FORMATO COMANDI (case-insensitive, parole abbreviabili per prefisso)
 * ------------------------------------------------------------------------
 * Ogni "parola chiave" puo' essere scritta per intero o abbreviata con le
 * sue prime lettere, purche' il prefisso sia univoco rispetto alla parola
 * stessa (es. "en" per "enable", "e" per "effect" come primo token, "g" per
 * "get", ecc). Non serve configurare nulla: il matching e' sempre a prefisso.
 *
 *   effect <id|nome>                          -> info + parametri (compatti)
 *   effect <id|nome> params                    -> lista parametri
 *   effect <id|nome> params reset              -> reset di tutti i parametri
 *   effect <id|nome> param <nome> get          -> info dettagliata parametro
 *   effect <id|nome> param <nome> set <valore> -> imposta un parametro
 *   effect <id|nome> param <nome> reset        -> reset singolo parametro
 *   effect <id|nome> enable                    -> abilita l'effetto
 *   effect <id|nome> disable                   -> disabilita l'effetto
 *
 *   list                                       -> elenco effetti in catena
 *   help                                       -> riepilogo comandi
 *
 * Esempi abbreviati equivalenti:
 *   "effect 2 params"        == "e 2 p"
 *   "effect 2 params reset"  == "e 2 p r"
 *   "effect delay param mix get"   == "e del p mix g"
 *   "effect delay param mix set 40" == "e del p mix s 40"
 *   "effect 0 enable"        == "e 0 en"
 *   "effect 0 disable"       == "e 0 di"
 *
 * Nota su "param" vs "params": non serve distinguerli per lettera, la
 * struttura del comando decide da sola: se dopo "p..." c'e' un nome di
 * parametro si tratta di "param" (singolo), altrimenti di "params" (lista).
 *
 * Anche il nome dell'effetto e del parametro possono essere abbreviati per
 * prefisso (es. "del" per "delay"), a patto che il prefisso sia univoco tra
 * gli effetti/parametri disponibili: viene restituito il primo che
 * corrisponde, in ordine di catena.
 *
 * ------------------------------------------------------------------------
 * FORMATO RISPOSTE (una riga per risposta, campi separati da '|')
 * ------------------------------------------------------------------------
 *   EFFECT|<id>|<nome>|<info>|EN=<0/1>|PARAMS=<n>|p1=v1,p2=v2,...
 *   PARAMS|<id>|<nome>|p1=v1,p2=v2,...
 *   PARAM|<id>|<nome_effetto>|<nome_param>|<tipo>|<valore>|MIN=<..>|MAX=<..>|DEF=<..>|RW=<RW/RO>|<info>
 *   LIST|<n>|id0:nome0:en0,id1:nome1:en1,...
 *   OK|EFFECT|<id>|EN=<0/1>
 *   OK|PARAMS|<id>|RESET
 *   OK|PARAM|<id>|<nome_param>|SET|<valore>
 *   OK|PARAM|<id>|<nome_param>|RESET|<valore>
 *   ERR|<motivo>
 *
 * ------------------------------------------------------------------------
 * USO TIPICO
 * ------------------------------------------------------------------------
 *   SerialInterface<8> si(effectChain, Serial1); // Serial1 verso Nextion
 *   void setup() { Serial1.begin(115200); si.begin(); }
 *   void loop()  { si.update(); }
 *
 * Il terminatore di riga accettato e' '\n' oppure ';' (utile per Nextion,
 * spesso piu' comodo del newline). '\r' viene sempre ignorato.
 * ------------------------------------------------------------------------
 */

#pragma once
#include <Arduino.h>

template <uint8_t MAX_EFFECTS>
class SerialInterface
{
public:
    SerialInterface(EffectChain<MAX_EFFECTS>& chain, Stream& stream = Serial)
        : _chain(chain), _stream(stream) {}

    void begin()
    {
        _buffer = "";
        _buffer.reserve(80);
    }

    // Da chiamare nel loop(): legge caratteri disponibili e processa i
    // comandi completi (terminati da '\n' o ';').
    void update()
    {
        while (_stream.available())
        {
            char c = _stream.read();
            if (c == '\n' || c == ';')
            {
                if (_buffer.length() > 0)
                {
                    processLine(_buffer);
                    _buffer = "";
                }
            }
            else if (c == '\r')
            {
                // ignorato
            }
            else if (_buffer.length() < 96)
            {
                _buffer += c;
            }
        }
    }

    // Espone il parsing anche per l'uso diretto/test (bypassando lo Stream).
    void processLine(const String& lineIn)
    {
        String tokens[MAX_TOKENS];
        uint8_t n = tokenize(lineIn, tokens, MAX_TOKENS);
        if (n == 0) return;

        if (matchesPrefix(tokens[0], "help"))
        {
            printHelp();
            return;
        }
        if (matchesPrefix(tokens[0], "list"))
        {
            printEffectsList();
            return;
        }
        if (!matchesPrefix(tokens[0], "effect"))
        {
            printError(F("comando sconosciuto"));
            return;
        }
        if (n < 2)
        {
            printError(F("id/nome effetto mancante"));
            return;
        }

        EffectRef er = resolveEffect(tokens[1]);
        if (!er.valid)
        {
            printError(F("effetto non trovato"));
            return;
        }

        if (n == 2)
        {
            printEffectInfo(er);
            return;
        }

        const String& t2 = tokens[2];

        if (matchesPrefix(t2, "enable"))
        {
            _chain.setEffectEnabled(er.id, true);
            printOkEnable(er);
            return;
        }
        if (matchesPrefix(t2, "disable"))
        {
            _chain.setEffectEnabled(er.id, false);
            printOkEnable(er);
            return;
        }

        if (matchesPrefix(t2, "params"))
        {
            // "params" (lista) oppure "params reset" (reset totale)
            if (n == 3)
            {
                printParamsList(er);
                return;
            }
            if (n == 4 && matchesPrefix(tokens[3], "reset"))
            {
                er.effect->parameterManager.resetAll();
                er.effect->updateParams();
                printOkParamsReset(er);
                return;
            }
            // Se arriviamo qui, tokens[3] non e' "reset": e' un nome di
            // parametro -> modalita' "param" singolo.
            handleSingleParam(er, tokens, n);
            return;
        }

        printError(F("sotto-comando sconosciuto"));
    }

private:
    static const uint8_t MAX_TOKENS = 6;

    struct EffectRef
    {
        AudioEffect* effect = nullptr;
        uint8_t id = 0;
        bool valid = false;
    };

    EffectChain<MAX_EFFECTS>& _chain;
    Stream& _stream;
    String _buffer;

    // ---------------------------------------------------------------- //
    // Parsing helpers
    // ---------------------------------------------------------------- //

    uint8_t tokenize(const String& line, String tokens[], uint8_t maxTokens)
    {
        uint8_t count = 0;
        int start = 0;
        int len = line.length();
        while (start < len && count < maxTokens)
        {
            while (start < len && line[start] == ' ') start++;
            if (start >= len) break;
            int end = start;
            while (end < len && line[end] != ' ') end++;
            tokens[count++] = line.substring(start, end);
            start = end;
        }
        return count;
    }

    // token e' un prefisso (case-insensitive, non vuoto) di word?
    bool matchesPrefix(const String& token, const char* word)
    {
        size_t tl = token.length();
        if (tl == 0) return false;
        size_t wl = strlen(word);
        if (tl > wl) return false;
        for (size_t i = 0; i < tl; i++)
        {
            char a = token[i];
            char b = word[i];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) return false;
        }
        return true;
    }

    bool isNumeric(const String& s)
    {
        if (s.length() == 0) return false;
        for (size_t i = 0; i < s.length(); i++)
        {
            if (!isDigit(s[i])) return false;
        }
        return true;
    }

    // ---------------------------------------------------------------- //
    // Risoluzione effetto / parametro (per id, nome esatto o prefisso)
    // ---------------------------------------------------------------- //

    EffectRef resolveEffect(const String& token)
    {
        EffectRef ref;

        if (isNumeric(token))
        {
            uint8_t id = (uint8_t)token.toInt();
            AudioEffect* eff = _chain.get(id);
            if (eff)
            {
                ref.effect = eff;
                ref.id = id;
                ref.valid = true;
            }
            return ref;
        }

        // 1) match esatto per nome
        AudioEffect* eff = _chain.getByName(token.c_str());
        if (eff)
        {
            uint8_t n = _chain.size();
            for (uint8_t i = 0; i < n; i++)
            {
                uint8_t id = _chain.getOrder(i);
                if (_chain.get(id) == eff)
                {
                    ref.effect = eff;
                    ref.id = id;
                    ref.valid = true;
                    return ref;
                }
            }
        }

        // 2) fallback: match per prefisso, primo trovato in ordine di catena
        uint8_t n = _chain.size();
        for (uint8_t i = 0; i < n; i++)
        {
            uint8_t id = _chain.getOrder(i);
            AudioEffect* e = _chain.get(id);
            if (e && matchesPrefix(token, e->getName()))
            {
                ref.effect = e;
                ref.id = id;
                ref.valid = true;
                return ref;
            }
        }

        return ref; // invalid
    }

    Param* resolveParam(AudioEffect* eff, const String& token)
    {
        if (!eff) return nullptr;

        Param* p = eff->parameterManager.find(token.c_str());
        if (p) return p;

        uint8_t cnt = eff->parameterManager.count();
        for (uint8_t i = 0; i < cnt; i++)
        {
            Param* pi = eff->parameterManager.at(i);
            if (pi && matchesPrefix(token, pi->name())) return pi;
        }
        return nullptr;
    }

    // ---------------------------------------------------------------- //
    // Gestione "effect <id> param <nome> <get|set|reset> [valore]"
    // ---------------------------------------------------------------- //

    void handleSingleParam(EffectRef& er, String tokens[], uint8_t n)
    {
        if (n < 4)
        {
            printError(F("nome parametro mancante"));
            return;
        }

        Param* p = resolveParam(er.effect, tokens[3]);
        if (!p)
        {
            printError(F("parametro non trovato"));
            return;
        }

        if (n < 5)
        {
            printError(F("azione mancante (get/set/reset)"));
            return;
        }

        const String& action = tokens[4];

        if (matchesPrefix(action, "get"))
        {
            printParamInfo(er, p);
            return;
        }

        if (matchesPrefix(action, "set"))
        {
            if (n < 6)
            {
                printError(F("valore mancante"));
                return;
            }
            bool ok = p->set(tokens[5]);
            if (ok)
            {
                er.effect->updateParams();
                printOkParamSet(er, p);
            }
            else
            {
                printError(F("valore non valido"));
            }
            return;
        }

        if (matchesPrefix(action, "reset"))
        {
            p->reset();
            er.effect->updateParams();
            printOkParamReset(er, p);
            return;
        }

        printError(F("azione sconosciuta"));
    }

    // ---------------------------------------------------------------- //
    // Output
    // ---------------------------------------------------------------- //

    void printEffectInfo(EffectRef& er)
    {
        ParameterManager& pm = er.effect->parameterManager;
        uint8_t cnt = pm.count();

        _stream.print(F("EFFECT|"));
        _stream.print(er.id);
        _stream.print('|');
        _stream.print(er.effect->getName());
        // _stream.print('|');
        // _stream.print(er.effect->getInfo());
        _stream.print('|');
        _stream.print(F("EN="));
        _stream.print(er.effect->isEnabled() ? 1 : 0);
        _stream.print('|');
        _stream.print(F("PARAMS="));
        _stream.print(cnt);
        _stream.print('|');
        printParamsCompact(pm, cnt);
        _stream.println();
    }

    void printParamsList(EffectRef& er)
    {
        ParameterManager& pm = er.effect->parameterManager;
        uint8_t cnt = pm.count();

        _stream.print(F("PARAMS|"));
        _stream.print(er.id);
        _stream.print('|');
        _stream.print(er.effect->getName());
        _stream.print('|');
        printParamsCompact(pm, cnt);
        _stream.println();
    }

    void printParamsCompact(ParameterManager& pm, uint8_t cnt)
    {
        for (uint8_t i = 0; i < cnt; i++)
        {
            Param* p = pm.at(i);
            if (!p) continue;
            _stream.print(p->name());
            _stream.print('=');
            _stream.print(p->get());
            if (i < cnt - 1) _stream.print(',');
        }
    }

    void printParamInfo(EffectRef& er, Param* p)
    {
        _stream.print(F("PARAM|"));
        _stream.print(er.id);
        _stream.print('|');
        _stream.print(er.effect->getName());
        _stream.print('|');
        _stream.print(p->name());
        _stream.print('|');
        _stream.print(paramTypeName(p->getType()));
        _stream.print('|');
        _stream.print(p->get());
        _stream.print('|');
        _stream.print(F("MIN="));
        _stream.print(p->minStr());
        _stream.print('|');
        _stream.print(F("MAX="));
        _stream.print(p->maxStr());
        _stream.print('|');
        _stream.print(F("DEF="));
        _stream.print(p->defaultStr());
        _stream.print('|');
        _stream.print(F("RW="));
        _stream.print(p->isSettable() ? "RW" : "RO");
        _stream.print('|');
        _stream.println(p->info());
    }

    void printEffectsList()
    {
        uint8_t n = _chain.size();
        _stream.print(F("LIST|"));
        _stream.print(n);
        _stream.print('|');
        for (uint8_t i = 0; i < n; i++)
        {
            uint8_t id = _chain.getOrder(i);
            AudioEffect* eff = _chain.get(id);
            if (!eff) continue;
            _stream.print(id);
            _stream.print(':');
            _stream.print(eff->getName());
            _stream.print(':');
            _stream.print(eff->isEnabled() ? 1 : 0);
            if (i < n - 1) _stream.print(',');
        }
        _stream.println();
    }

    void printHelp()
    {
        _stream.println(F("effect <id|nome> [params [reset] | param <nome> <get|set <v>|reset> | enable | disable]"));
        _stream.println(F("list"));
    }

    void printOkEnable(EffectRef& er)
    {
        _stream.print(F("OK|EFFECT|"));
        _stream.print(er.id);
        _stream.print(F("|EN="));
        _stream.println(er.effect->isEnabled() ? 1 : 0);
    }

    void printOkParamsReset(EffectRef& er)
    {
        _stream.print(F("OK|PARAMS|"));
        _stream.print(er.id);
        _stream.println(F("|RESET"));
    }

    void printOkParamSet(EffectRef& er, Param* p)
    {
        _stream.print(F("OK|PARAM|"));
        _stream.print(er.id);
        _stream.print('|');
        _stream.print(p->name());
        _stream.print(F("|SET|"));
        _stream.println(p->get());
    }

    void printOkParamReset(EffectRef& er, Param* p)
    {
        _stream.print(F("OK|PARAM|"));
        _stream.print(er.id);
        _stream.print('|');
        _stream.print(p->name());
        _stream.print(F("|RESET|"));
        _stream.println(p->get());
    }

    void printError(const __FlashStringHelper* reason)
    {
        _stream.print(F("ERR|"));
        _stream.println(reason);
    }
};