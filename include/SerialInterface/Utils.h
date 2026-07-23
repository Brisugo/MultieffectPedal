#pragma once

#include <ctype.h>
#include <stdlib.h>

// ------------------------------------------------------------
// Helper di confronto stringhe (case-insensitive), non tutti i
// core Arduino offrono strcasecmp/strncasecmp
// ------------------------------------------------------------
namespace SerialInterfaceUtil {

inline bool equalsIgnoreCase(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
        a++; b++;
    }
    return *a == *b;
}

inline bool startsWithIgnoreCase(const char* full, const char* prefix) {
    while (*prefix) {
        if (!*full) return false;
        if (tolower((unsigned char)*full) != tolower((unsigned char)*prefix)) return false;
        full++; prefix++;
    }
    return true;
}

// Estrae le "iniziali" da un nome: prima lettera di ogni parola
// (separata da _ , - o spazio) più ogni lettera maiuscola interna
// (per gestire anche camelCase / PascalCase).
inline void extractInitials(const char* full, char* out, uint8_t maxLen) {
    uint8_t n = 0;
    bool startOfWord = true;
    for (const char* p = full; *p && n < (uint8_t)(maxLen - 1); p++) {
        char c = *p;
        if (c == '_' || c == '-' || c == ' ') { startOfWord = true; continue; }
        if (startOfWord || isupper((unsigned char)c)) {
            out[n++] = (char)toupper((unsigned char)c);
            startOfWord = false;
        } else {
            startOfWord = false;
        }
    }
    out[n] = '\0';
    if (n == 0 && full[0]) { out[0] = (char)toupper((unsigned char)full[0]); out[1] = '\0'; }
}

// full = nome completo (effetto/parametro/comando)
// abbrev = quanto digitato dall'utente
inline bool matchesAbbrev(const char* full, const char* abbrev) {
    if (!full || !abbrev || !*abbrev) return false;
    if (equalsIgnoreCase(full, abbrev)) return true;
    if (startsWithIgnoreCase(full, abbrev)) return true;
    char initials[8];
    extractInitials(full, initials, sizeof(initials));
    if (equalsIgnoreCase(initials, abbrev)) return true;
    return false;
}

inline bool isAllDigits(const char* s) {
    if (!s || !*s) return false;
    for (const char* p = s; *p; p++) if (!isdigit((unsigned char)*p)) return false;
    return true;
}

} // namespace SerialInterfaceUtil