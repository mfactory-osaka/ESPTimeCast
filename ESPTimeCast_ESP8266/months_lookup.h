#ifndef MONTHS_LOOKUP_H
#define MONTHS_LOOKUP_H

typedef struct {
    const char* lang;
    const char* months[12]; // Jan to Dec
} MonthsMapping;

const MonthsMapping months_mappings[] = {
    { "af", { "jan", "feb", "mar", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "des" } }, // Afrikaans
    { "cs", { "led", "uno", "bre", "dub", "kve", "cer", "cvc", "srp", "zar", "rij", "lis", "pro" } }, // Czech
    { "da", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Danish
    { "de", { "jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "dez" } }, // German
    { "en", { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" } }, // English
    { "eo", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Esperanto
    { "es", { "ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic" } }, // Spanish
    { "et", { "jan", "veb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "det" } }, // Estonian
    { "fi", { "tam", "hel", "maa", "huh", "tou", "kes", "hei", "elo", "syy", "lok", "mar", "jou" } }, // Finnish
    { "fr", { "jan", "fev", "mar", "avr", "mai", "jun", "jul", "aou", "sep", "oct", "nov", "dec" } }, // French
    { "hr", { "sij", "vel", "ozu", "tra", "svi", "lip", "srp", "kol", "ruj", "lis", "stu", "pro" } }, // Croatian
    { "hu", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sze", "okt", "nov", "dec" } }, // Hungarian
    { "it", { "gen", "feb", "mar", "apr", "mag", "giu", "lug", "ago", "set", "ott", "nov", "dic" } }, // Italian
    { "ga", { "ean", "fea", "mar", "aib", "bea", "mei", "iui", "lun", "mea", "dei", "sam", "nol" } }, // Irish
    { "ja", { "1 ²", "2 ²", "3 ²", "4 ²", "5 ²", "6 ²", "7 ²", "8 ²", "9 ²", "10 ²", "11 ²", "12 ²" } }, // Japanese
    { "lt", { "sau", "vas", "kov", "bal", "geg", "bir", "lie", "rug", "swe", "spa", "lap", "gru" } }, // Lithuanian
    { "lv", { "jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Latvian
    { "nl", { "jan", "feb", "maa", "apr", "mei", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Dutch
    { "no", { "jan", "feb", "mar", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "des" } }, // Norwegian
    { "pl", { "sty", "lut", "mar", "kwi", "maj", "cze", "lip", "sie", "wrz", "paz", "lis", "gru" } }, // Polish
    { "pt", { "jan", "fev", "mar", "abr", "mai", "jun", "jul", "ago", "set", "out", "nov", "dez" } }, // Portuguese
    { "ro", { "ian", "feb", "mar", "apr", "mai", "iun", "iul", "aug", "sep", "oct", "nov", "dec" } }, // Romanian
    { "ru", { "ian", "feb", "mar", "apr", "mai", "iun", "iul", "aug", "sep", "oct", "noi", "dec" } }, // Russian
    { "sk", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Slovak
    { "sl", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "avg", "sep", "okt", "nov", "dec" } }, // Slovenian
    { "sr", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "avg", "sep", "okt", "nov", "dec" } }, // Serbian
    { "sv", { "jan", "feb", "mar", "apr", "maj", "jun", "jul", "aug", "sep", "okt", "nov", "dec" } }, // Swedish
    { "sw", { "jan", "feb", "mar", "apr", "mei", "jun", "jul", "ago", "sep", "okt", "nov", "des" } }, // Swahili
    { "tr", { "oca", "sub", "mar", "nis", "may", "haz", "tem", "agu", "eyl", "eki", "kas", "ara" } }  // Turkish
};

#define MONTHS_MAPPINGS_COUNT (sizeof(months_mappings)/sizeof(months_mappings[0]))

inline const char* const* getMonthsOfYear(const char* lang) {
    for (size_t i = 0; i < MONTHS_MAPPINGS_COUNT; i++) {
        if (strcmp(lang, months_mappings[i].lang) == 0)
            return months_mappings[i].months;
    }
    // fallback to English if not found
    return months_mappings[4].months; // "en" is index 4
}

#endif // MONTHS_LOOKUP_H