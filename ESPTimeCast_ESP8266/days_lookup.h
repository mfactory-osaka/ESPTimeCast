#ifndef DAYS_LOOKUP_H
#define DAYS_LOOKUP_H

typedef struct {
    const char* lang;
    const char* days[7]; 
} DaysOfWeekMapping;

const DaysOfWeekMapping days_mappings[] = {
    { "af", { "sun", "maa", "din", "woe", "don", "vry", "son" } },
    { "cs", { "ned", "pon", "ute", "str", "ctv", "pat", "sob" } },
    { "da", { "son", "man", "tir", "ons", "tor", "fre", "lor" } },
    { "de", { "so", "mo", "di", "mi", "do", "fr", "sa" } },
    { "en", { "sun", "mon", "tue", "wed", "thu", "fri", "sat" } },
    { "eo", { "dim", "lun", "mar", "mer", "jau", "ven", "sab" } },
    { "es", { "dom", "lun", "mar", "mie", "jue", "vie", "sab" } },
    { "et", { "pa", "es", "te", "ko", "ne", "re", "la" } },
    { "fi", { "sun", "maa", "tis", "kes", "tor", "per", "lau" } },
    { "fr", { "dim", "lun", "mar", "mer", "jeu", "ven", "sam" } },
    { "ga", { "dom", "lua", "mai", "cea", "dea", "aoi", "sat" } },
    { "hr", { "ned", "pon", "uto", "sri", "cet", "pet", "sub" } },
    { "hu", { "vas", "het", "ked", "sze", "csu", "pet", "szo" } },
    { "it", { "dom", "lun", "mar", "mer", "gio", "ven", "sab" } },
    { "ja", { "±", "²", "³", "´", "µ", "¶", "·" } },
    { "lt", { "sek", "pir", "ant", "tre", "ket", "pen", "ses" } },
    { "lv", { "sve", "pir", "otr", "tre", "cet", "pie", "ses" } },
    { "nl", { "zon", "maa", "din", "woe", "don", "vri", "zat" } },
    { "no", { "son", "man", "tir", "ons", "tor", "fre", "lor" } },
    { "pl", { "nie", "pon", "wto", "sro", "czw", "pia", "sob" } },
    { "pt", { "dom", "seg", "ter", "qua", "qui", "sex", "sab" } },
    { "ro", { "dum", "lun", "mar", "mie", "joi", "vin", "sam" } },
    { "ru", { "bc", "nh", "bt", "cp", "\x84t", "nt", "c\x85" } },
    { "sk", { "ned", "pon", "uto", "str", "stv", "pia", "sob" } },
    { "sl", { "ned", "pon", "tor", "sre", "cet", "pet", "sob" } },
    { "sr", { "ned", "pon", "uto", "sre", "cet", "pet", "sub" } },
    { "sv", { "son", "man", "tis", "ons", "tor", "fre", "lor" } },
    { "sw", { "jpl", "jum", "jtt", "jtn", "alk", "ijm", "jms" } },
    { "tr", { "paz", "paz", "sal", "car", "per", "cum", "cum" } }
};

#define DAYS_MAPPINGS_COUNT (sizeof(days_mappings)/sizeof(days_mappings[0]))

inline const char* const* getDaysOfWeek(const char* lang) {
    for (size_t i = 0; i < DAYS_MAPPINGS_COUNT; i++) {
        if (strcmp(lang, days_mappings[i].lang) == 0)
            return days_mappings[i].days;
    }
    return days_mappings[4].days; // fallback "en"
}

#endif