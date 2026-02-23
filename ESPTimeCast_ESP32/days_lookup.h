#ifndef DAYS_LOOKUP_H
#define DAYS_LOOKUP_H

typedef struct {
    const char* lang;
    const char* days[7]; // Sunday to Saturday (tm_wday order)
} DaysOfWeekMapping;

const DaysOfWeekMapping days_mappings[] = {
    { "af", { "s\016u\016n", "m\016a\016a", "d\016i\016n", "w\016o\016e", "d\016o\016n", "v\016r\016y", "s\016o\016n" } },
    { "cs", { "n\016e\016d", "p\016o\016n", "u\016t\016e", "s\016t\016r", "c\016t\016v", "p\016a\016t", "s\016o\016b" } },
    { "da", { "s\016o\016n", "m\016a\016n", "t\016i\016r", "o\016n\016s", "t\016o\016r", "f\016r\016e", "l\016o\016r" } },
    { "de", { "s\016o", "m\016o", "d\016i", "m\016i", "d\016o", "f\016r", "s\016a" } },
    { "en", { "s\016u\016n", "m\016o\016n", "t\016u\016e", "w\016e\016d", "t\016h\016u", "f\016r\016i", "s\016a\016t" } },
    { "eo", { "d\016i\016m", "l\016u\016n", "m\016a\016r", "m\016e\016r", "j\016a\016u", "v\016e\016n", "s\016a\016b" } },
    { "es", { "d\016o\016m", "l\016u\016n", "m\016a\016r", "m\016i\016e", "j\016u\016e", "v\016i\016e", "s\016a\016b" } },
    { "et", { "p\016a", "e\016s", "t\016e", "k\016o", "n\016e", "r\016e", "l\016a" } },
    { "fi", { "s\016u\016n", "m\016a\016a", "t\016i\016s", "k\016e\016s", "t\016o\016r", "p\016e\016r", "l\016a\016u" } },
    { "fr", { "d\016i\016m", "l\016u\016n", "m\016a\016r", "m\016e\016r", "j\016e\016u", "v\016e\016n", "s\016a\016m" } },
    { "ga", { "d\016o\016m", "l\016u\016a", "m\016a\016i", "c\016e\016a", "d\016e\016a", "a\016o\016i", "s\016a\016t" } },
    { "hr", { "n\016e\016d", "p\016o\016n", "u\016t\016o", "s\016r\016i", "c\016e\016t", "p\016e\016t", "s\016u\016b" } },
    { "hu", { "v\016a\016s", "h\016e\016t", "k\016e\016d", "s\016z\016e", "c\016s\016u", "p\016e\016t", "s\016z\016o" } },
    { "it", { "d\016o\016m", "l\016u\016n", "m\016a\016r", "m\016e\016r", "g\016i\016o", "v\016e\016n", "s\016a\016b" } },
    { "ja", { "±", "²", "³", "´", "µ", "¶", "·" } },
    { "lt", { "s\016e\016k", "p\016i\016r", "a\016n\016t", "t\016r\016e", "k\016e\016t", "p\016e\016n", "s\016e\016s" } },
    { "lv", { "s\016v\016e", "p\016i\016r", "o\016t\016r", "t\016r\016e", "c\016e\016t", "p\016i\016e", "s\016e\016s" } },
    { "nl", { "z\016o\016n", "m\016a\016a", "d\016i\016n", "w\016o\016e", "d\016o\016n", "v\016r\016i", "z\016a\016t" } },
    { "no", { "s\016o\016n", "m\016a\016n", "t\016i\016r", "o\016n\016s", "t\016o\016r", "f\016r\016e", "l\016o\016r" } },
    { "pl", { "n\016i\016e", "p\016o\016n", "w\016t\016o", "s\016r\016o", "c\016z\016w", "p\016i\016a", "s\016o\016b" } },
    { "pt", { "d\016o\016m", "s\016e\016g", "t\016e\016r", "q\016u\016a", "q\016u\016i", "s\016e\016x", "s\016a\016b" } },
    { "ro", { "d\016u\016m", "l\016u\016n", "m\016a\016r", "m\016i\016e", "j\016o\016i", "v\016i\016n", "s\016a\016m" } },
    { "ru", { "b\016c", "n\016h", "b\016t", "c\016p", "\x84\016t", "n\016t", "c\016\x85" } },
    { "sk", { "n\016e\016d", "p\016o\016n", "u\016t\016o", "s\016t\016r", "s\016t\016v", "p\016i\016a", "s\016o\016b" } },
    { "sl", { "n\016e\016d", "p\016o\016n", "t\016o\016r", "s\016r\016e", "c\016e\016t", "p\016e\016t", "s\016o\016b" } },
    { "sr", { "n\016e\016d", "p\016o\016n", "u\016t\016o", "s\016r\016e", "c\016e\016t", "p\016e\016t", "s\016u\016b" } },
    { "sv", { "s\016o\016n", "m\016a\016n", "t\016i\016s", "o\016n\016s", "t\016o\016r", "f\016r\016e", "l\016o\016r" } },
    { "sw", { "j\016p\016l", "j\016u\016m", "j\016t\016t", "j\016t\016n", "a\016l\016k", "i\016j\016m", "j\016m\016s" } },
    { "tr", { "p\016a\016z", "p\016a\016z", "s\016a\016l", "c\016a\016r", "p\016e\016r", "c\016u\016m", "c\016u\016m" } }
};

#define DAYS_MAPPINGS_COUNT (sizeof(days_mappings)/sizeof(days_mappings[0]))

inline const char* const* getDaysOfWeek(const char* lang) {
    for (size_t i = 0; i < DAYS_MAPPINGS_COUNT; i++) {
        if (strcmp(lang, days_mappings[i].lang) == 0)
            return days_mappings[i].days;
    }
    // fallback to English if not found
    return days_mappings[4].days; // "en" is index 4
}

#endif // DAYS_LOOKUP_H