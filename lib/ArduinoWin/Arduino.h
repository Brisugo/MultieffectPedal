// Arduino.h — stub minimale del "core" Arduino per compilare lo sketch su PC (Windows).
// Fornisce solo cio' che serve allo sketch: millis()/delay(), le classi Print/Stream
// e un oggetto globale Serial collegato al terminale.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <string>
// #include <conio.h>   // _kbhit(), _getch() — Windows only

#define M_PI		3.14159265358979323846
#define F(a) a

void setup();
void loop();

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------
inline uint32_t millis() {
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - t0).count());
}

inline uint32_t micros() {
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - t0).count());
}

inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

inline void delayMicroseconds(uint32_t us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

// ---------------------------------------------------------------------------
// String — riproduce il sottoinsieme piu' usato della classe String di Arduino,
// appoggiandosi a std::string.
// ---------------------------------------------------------------------------
class String {
public:
    String() = default;
    String(const char* s) : _s(s ? s : "") {}
    String(const std::string& s) : _s(s) {}
    String(char c) : _s(1, c) {}
    String(int v)          { char b[32]; snprintf(b, sizeof(b), "%d", v);  _s = b; }
    String(unsigned int v) { char b[32]; snprintf(b, sizeof(b), "%u", v);  _s = b; }
    String(long v)         { char b[32]; snprintf(b, sizeof(b), "%ld", v); _s = b; }
    String(float v, int digits = 2)  { char b[64]; snprintf(b, sizeof(b), "%.*f", digits, v); _s = b; }
    String(double v, int digits = 2) { char b[64]; snprintf(b, sizeof(b), "%.*f", digits, v); _s = b; }

    const char* c_str() const { return _s.c_str(); }
    unsigned int length() const { return static_cast<unsigned int>(_s.size()); }
    bool isEmpty() const { return _s.empty(); }

    char charAt(unsigned int i) const { return i < _s.size() ? _s[i] : '\0'; }
    char operator[](unsigned int i) const { return charAt(i); }

    int indexOf(char c) const { auto p = _s.find(c); return p == std::string::npos ? -1 : (int)p; }
    int indexOf(const String& s) const { auto p = _s.find(s._s); return p == std::string::npos ? -1 : (int)p; }
    int indexOf(const char* s) const { auto p = _s.find(s); return p == std::string::npos ? -1 : (int)p; }

    String substring(unsigned int from) const { return String(_s.substr(from)); }
    String substring(unsigned int from, unsigned int to) const { return String(_s.substr(from, to - from)); }

    void trim() {
        size_t a = _s.find_first_not_of(" \t\r\n");
        size_t b = _s.find_last_not_of(" \t\r\n");
        _s = (a == std::string::npos) ? "" : _s.substr(a, b - a + 1);
    }

    void toUpperCase() { for (auto& c : _s) c = static_cast<char>(toupper((unsigned char)c)); }
    void toLowerCase() { for (auto& c : _s) c = static_cast<char>(tolower((unsigned char)c)); }

    int toInt() const { return atoi(_s.c_str()); }
    float toFloat() const { return static_cast<float>(atof(_s.c_str())); }

    bool equals(const String& other) const { return _s == other._s; }
    bool equalsIgnoreCase(const String& other) const {
        if (_s.size() != other._s.size()) return false;
        for (size_t i = 0; i < _s.size(); i++)
            if (tolower((unsigned char)_s[i]) != tolower((unsigned char)other._s[i])) return false;
        return true;
    }
    bool startsWith(const String& prefix) const { return _s.rfind(prefix._s, 0) == 0; }
    bool endsWith(const String& suffix) const {
        if (suffix._s.size() > _s.size()) return false;
        return _s.compare(_s.size() - suffix._s.size(), suffix._s.size(), suffix._s) == 0;
    }
    void replace(const String& from, const String& to) {
        if (from._s.empty()) return;
        size_t pos = 0;
        while ((pos = _s.find(from._s, pos)) != std::string::npos) {
            _s.replace(pos, from._s.size(), to._s);
            pos += to._s.size();
        }
    }

    String& operator+=(const String& other) { _s += other._s; return *this; }
    String operator+(const String& other) const { return String(_s + other._s); }
    bool operator==(const String& other) const { return _s == other._s; }
    bool operator!=(const String& other) const { return _s != other._s; }

    const std::string& stl() const { return _s; }

private:
    std::string _s;
};

// ---------------------------------------------------------------------------
// Print / Stream — stessa gerarchia del core Arduino, riscritta per il terminale
// ---------------------------------------------------------------------------
class Print {
public:
    virtual ~Print() {}

    virtual size_t write(uint8_t c) = 0;

    virtual size_t write(const uint8_t* buf, size_t size) {
        size_t n = 0;
        for (size_t i = 0; i < size; i++) n += write(buf[i]);
        return n;
    }

    size_t print(const char* s)              { return write(reinterpret_cast<const uint8_t*>(s), strlen(s)); }
    size_t print(const String& s)             { return write(reinterpret_cast<const uint8_t*>(s.c_str()), s.length()); }
    size_t print(char c)                      { return write(static_cast<uint8_t>(c)); }
    size_t print(int v)                       { char b[32]; int n = snprintf(b, sizeof(b), "%d", v); return write(reinterpret_cast<const uint8_t*>(b), n); }
    size_t print(unsigned int v)              { char b[32]; int n = snprintf(b, sizeof(b), "%u", v); return write(reinterpret_cast<const uint8_t*>(b), n); }
    size_t print(long v)                      { char b[32]; int n = snprintf(b, sizeof(b), "%ld", v); return write(reinterpret_cast<const uint8_t*>(b), n); }
    size_t print(float v, int digits = 2)     { char b[64]; int n = snprintf(b, sizeof(b), "%.*f", digits, v); return write(reinterpret_cast<const uint8_t*>(b), n); }
    size_t print(double v, int digits = 2)    { return print(static_cast<float>(v), digits); }

    size_t println()                          { return write(reinterpret_cast<const uint8_t*>("\r\n"), 2); }
    size_t println(const char* s)             { size_t n = print(s); n += println(); return n; }
    size_t println(const String& s)           { size_t n = print(s); n += println(); return n; }
    size_t println(char c)                    { size_t n = print(c); n += println(); return n; }
    size_t println(int v)                     { size_t n = print(v); n += println(); return n; }
    size_t println(float v, int digits = 2)   { size_t n = print(v, digits); n += println(); return n; }
};

class Stream : public Print {
public:
    virtual ~Stream() {}
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() {}
};

// ---------------------------------------------------------------------------
// Serial "virtuale" collegato al terminale.
// available()/read() sono non bloccanti (usano _kbhit), come su Arduino.
// ---------------------------------------------------------------------------
class ConsoleSerial : public Stream {
public:
    void begin(unsigned long /*baud*/) {}

    int available() override {
        return (_peeked != -1) ? 1 : 1;
    }

    bool riddin = false;
    int read() override {
        if (_peeked != -1) {
            int c = _peeked;
            _peeked = -1;
            if (c == '\r') c = '\n';
            write(c);
            return c;
        }

        if(!riddin) printf(">>> ");

        int c = getchar();   // bloccante
        riddin = true;

        if (c == EOF){
            riddin = false;
            return -1;
        }

        if (c == '\r' || c == '\n'){
            riddin = false;
            c = '\n';
        }

        return c;
    }

    int peek() override {
        if (_peeked == -1) {
            _peeked = getchar();   // bloccante
        }
        return _peeked;
    }

    size_t write(uint8_t c) override {
        putchar(c);
        fflush(stdout);
        return 1;
    }

private:
    int _peeked = -1;
};

ConsoleSerial Serial;


// Equivalente PC di main()/HAL_init() di Arduino: chiama setup() una volta
// e poi loop() ripetutamente, come fa il runtime di Arduino.
int main() {
    setup();
    while (true) {
        loop();
        delay(1); // evita di saturare una CPU core inutilmente
    }
    return 0;
}