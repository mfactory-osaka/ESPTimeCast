#ifndef DAYS_LOOKUP_H
#define DAYS_LOOKUP_H

typedef struct {
    const char* lang;
    const char* days[7]; // Sunday to Saturday (tm_wday order)
} DaysOfWeekMapping;

const DaysOfWeekMapping days_mappings[] = {
    { "af", { "s&u&n", "m&a&a", "d&i&n", "w&o&e", "d&o&n", "v&r&y", "s&o&n" } },
    { "cs", { "n&e&d", "p&o&n", "u&t&e", "s&t&r", "c&t&v", "p&a&t", "s&o&b" } },
    { "da", { "s&o&n", "m&a&n", "t&i&r", "o&n&s", "t&o&r", "f&r&e", "l&o&r" } },
    { "de", { "s&o", "m&o", "d&i", "m&i", "d&o", "f&r", "s&a" } },
    { "en", { "s&u&n", "m&o&n", "t&u&e", "w&e&d", "t&h&u", "f&r&i", "s&a&t" } },
    { "eo", { "d&i&m", "l&u&n", "m&a&r", "m&e&r", "j&a&u", "v&e&n", "s&a&b" } },
    { "es", { "d&o&m", "l&u&n", "m&a&r", "m&i&e", "j&u&e", "v&i&e", "s&a&b" } },
    { "et", { "p&a",   "e&s",   "t&e",   "k&o",   "n&e",   "r&e",   "l&a" } },
    { "fi", { "s&u&n", "m&a&a", "t&i&s", "k&e&s", "t&o&r", "p&e&r", "l&a&u" } },
    { "fr", { "d&i&m", "l&u&n", "m&a&r", "m&e&r", "j&e&u", "v&e&n", "s&a&m" } },
    { "ga", { "d&o&m", "l&u&a", "m&a&i", "c&e&a", "d&e&a", "a&o&i", "s&a&t" } },
    { "hr", { "n&e&d", "p&o&n", "u&t&o", "s&r&i", "c&e&t", "p&e&t", "s&u&b" } },
    { "hu", { "v&a&s", "h&e&t", "k&e&d", "s&z&e", "c&s&u", "p&e&t", "s&z&o" } },
    { "it", { "d&o&m", "l&u&n", "m&a&r", "m&e&r", "g&i&o", "v&e&n", "s&a&b" } },
    { "ja", { "±", "²", "³", "´", "µ", "¶", "·" } },
    { "lt", { "s&e&k", "p&i&r", "a&n&t", "t&r&e", "k&e&t", "p&e&n", "s&e&s" } },
    { "lv", { "s&v&e", "p&i&r", "o&t&r", "t&r&e", "c&e&t", "p&i&e", "s&e&s" } },
    { "nl", { "z&o&n", "m&a&a", "d&i&n", "w&o&e", "d&o&n", "v&r&i", "z&a&t" } },
    { "no", { "s&o&n", "m&a&n", "t&i&r", "o&n&s", "t&o&r", "f&r&e", "l&o&r" } },
    { "pl", { "n&i&e", "p&o&n", "w&t&o", "s&r&o", "c&z&w", "p&i&a", "s&o&b" } },
    { "pt", { "d&o&m", "s&e&g", "t&e&r", "q&u&a", "q&u&i", "s&e&x", "s&a&b" } },
    { "ro", { "d&u&m", "l&u&n", "m&a&r", "m&i&e", "j&o&i", "v&i&n", "s&a&m" } },
    { "sk", { "n&e&d", "p&o&n", "u&t&o", "s&t&r", "s&t&v", "p&i&a", "s&o&b" } },
    { "sl", { "n&e&d", "p&o&n", "t&o&r", "s&r&e", "c&e&t", "p&e&t", "s&o&b" } },
    { "sr", { "n&e&d", "p&o&n", "u&t&o", "s&r&e", "c&e&t", "p&e&t", "s&u&b" } },
    { "sv", { "s&o&n", "m&a&n", "t&i&s", "o&n&s", "t&o&r", "f&r&e", "l&o&r" } },
    { "sw", { "j&p&l", "j&u&m", "j&t&t", "j&t&n", "a&l&k", "i&j&m", "j&m&s" } },
    { "tr", { "p&a&z", "p&a&z", "s&a&l", "c&a&r", "p&e&r", "c&u&m", "c&u&m" } }
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