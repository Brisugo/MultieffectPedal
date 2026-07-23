#pragma once
#include "Arduino.h"

// =====================================================================
// Parameter.h - Gestione parametri configurabili per Arduino
// Compatibile con Arduino UNO R4 Minima
// Versione con dispatch a runtime (if/else su ParamType), classe
// concreta Param (non template) per poterla mettere direttamente
// in array eterogenei senza puntatori/classi base.
// =====================================================================
 
enum class ParamType : uint8_t {
    INT,
    FLOAT,
    BOOL,
    STRING,
    UNKNOWN
};
 
 
template <typename T>
struct ParamStruct{
    char* name;

    bool settable = false;
    
    T default_value;
    T min;
    T max;

    char* info;
};
 

inline String paramTypeName(ParamType t) {
    switch (t) {
        case ParamType::INT:    return "int";
        case ParamType::FLOAT:  return "float";
        case ParamType::BOOL:   return "bool";
        case ParamType::STRING: return "string";
        default:                return "unknown";
    }
}
 
// ---- Conversioni tipizzate (specializzabili per nuovi tipi) --------
 
template<typename T>
inline T& getRef(void* ref) { return *(T*)ref; }

template<typename T>
inline const T& getCRef(const void* ref) { return *(T*)ref; }

template<typename T> ParamType paramTypeOf();
template<> inline ParamType paramTypeOf<int>()    { return ParamType::INT; }
template<> inline ParamType paramTypeOf<float>()  { return ParamType::FLOAT; }
template<> inline ParamType paramTypeOf<bool>()   { return ParamType::BOOL; }
template<> inline ParamType paramTypeOf<String>() { return ParamType::STRING; }
  
template<typename T> String paramToString(const void* ref);
template<> inline String paramToString<int>(const void* ref)    { return String(*(const int*)ref); }
template<> inline String paramToString<float>(const void* ref)  { return String(*(const float*)ref, 1); }
template<> inline String paramToString<bool>(const void* ref)   { return (*(const bool*)ref) ? "true" : "false"; }
template<> inline String paramToString<String>(const void* ref) { return *(const String*)ref; }


template<typename T> T paramFromString(const String& s);
template<> inline int paramFromString<int>(const String& s)       { return s.toInt(); }
template<> inline float paramFromString<float>(const String& s)   { return s.toFloat(); }
template<> inline bool paramFromString<bool>(const String& s)     { return (s == "1" || s.equalsIgnoreCase("true")); }
template<> inline String paramFromString<String>(const String& s) { return s; }
 
// ---- Param: classe concreta, NON template. Il tipo reale del ------
// ---- parametro e' salvato a runtime in 'type' e usato con if/else -
 
class Param {
public:
    // Param() : ref(nullptr), name_(""), info_(""), type(ParamType::UNKNOWN), settable(true) {}
 
    Param() {}

    // Sintassi di creazione (un parametro per riga):
    // Param p(variabile, "nome", min, max, default, "info", settable);
    template<typename T>
    Param(void* ref, const ParamStruct<T>& data) {
        this->ref = ref;
        type = paramTypeOf<T>();
        
        name_ = data.name;
        info_ = data.info;
        settable = data.settable;

        v_max = &data.max;
        v_min = &data.min;
        v_default = &data.default_value;

        *(T*)ref = *(T*)v_default;
    }
 
    const char* name() const { return name_; }
    const char* info() const { return info_; }
    ParamType getType() const { return type; }
    bool isSettable() const { return settable; }
 
    String get() const {
        if (type == ParamType::INT)    return paramToString<int>(ref);
        if (type == ParamType::FLOAT)  return paramToString<float>(ref);
        if (type == ParamType::BOOL)   return paramToString<bool>(ref);
        if (type == ParamType::STRING) return paramToString<String>(ref);
        return "";
    }
 
    // ritorna false se readOnly o se il valore e' fuori range
    bool set(const String& str) {
        if (!settable) return false;
 
        if (type == ParamType::INT) {
            int v = paramFromString<int>(str);
            if (v < getCRef<int>(v_min) || v > getCRef<int>(v_max)) return false;
            *(int*)ref = v;
        } else if (type == ParamType::FLOAT) {
            float v = paramFromString<float>(str);
            if (v < getCRef<float>(v_min) || v > getCRef<float>(v_max)) return false;
            getRef<float>(ref) = v;
        } else if (type == ParamType::BOOL) {
            getRef<bool>(ref) = paramFromString<bool>(str);
        } else if (type == ParamType::STRING) {
            getRef<String>(ref) = paramFromString<String>(str);
        } else {
            return false;
        }
        return true;
    }
 
    String minStr() const {          
        if (type == ParamType::INT)    return paramToString<int>(v_min);
        if (type == ParamType::FLOAT)  return paramToString<float>(v_min);
        if (type == ParamType::BOOL)   return paramToString<bool>(v_min);
        if (type == ParamType::STRING) return paramToString<String>(v_min);
        return "";
    }
    String maxStr() const {
        if (type == ParamType::INT)    return paramToString<int>(v_max);
        if (type == ParamType::FLOAT)  return paramToString<float>(v_max);
        if (type == ParamType::BOOL)   return paramToString<bool>(v_max);
        if (type == ParamType::STRING) return paramToString<String>(v_max);
        return "";
    }
    String defaultStr() const {
        if (type == ParamType::INT)    return paramToString<int>(v_default);
        if (type == ParamType::FLOAT)  return paramToString<float>(v_default);
        if (type == ParamType::BOOL)   return paramToString<bool>(v_default);
        if (type == ParamType::STRING) return paramToString<String>(v_default);
        return "";
    }
 
    void reset() {
        if (type == ParamType::INT)         getRef<int>(ref)    = getCRef<int>(v_default);
        else if (type == ParamType::FLOAT)  getRef<float>(ref)  = getCRef<float>(v_default);
        else if (type == ParamType::BOOL)   getRef<bool>(ref)   = getCRef<bool>(v_default);
        else if (type == ParamType::STRING) getRef<String>(ref) = getCRef<String>(v_default);
    }
 
private:
    void* ref;

    const char* name_;
    const char* info_;
    const void* v_max;
    const void* v_min;
    const void* v_default;

    ParamType type;

    bool settable;
};

class ParameterManager {
public:
    ParameterManager() {}

    ParameterManager(Param* list, uint8_t count) : _list(list), _count(count) {}
 
    Param* find(const char* paramName) {
        for (uint8_t i = 0; i < _count; i++) {
            if (strcmp(_list[i].name(), paramName) == 0) return &_list[i];
        }
        return nullptr;
    }
 
    bool setByName(const char* paramName, const String& value) {
        Param* p = find(paramName);
        if (!p) return false;
        return p->set(value);
    }
 
    String getByName(const char* paramName) {
        Param* p = find(paramName);
        if (!p) return String("");
        return p->get();
    }
 
    uint8_t count() const { return _count; }
    Param* at(uint8_t i) { return (i < _count) ? &_list[i] : nullptr; }
 
    void resetAll() {
        for (uint8_t i = 0; i < _count; i++) _list[i].reset();
    } 
private:
    Param* _list;
    uint8_t _count;
};
