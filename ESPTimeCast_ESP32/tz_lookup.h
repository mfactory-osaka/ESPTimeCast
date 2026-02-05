#ifndef TZ_LOOKUP_H
#define TZ_LOOKUP_H

typedef struct {
    const char* iana;
    const char* posix;
} TimeZoneMapping;

const TimeZoneMapping tz_mappings[] = {
    {"Africa/Cairo", "EET-2EEST,M4.5.5/0,M10.5.5/0"},
    {"Africa/Casablanca", "WET0WEST,M3.5.0/0,M10.5.0/0"},
    {"Africa/Johannesburg", "SAST-2"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
    {"America/Argentina/Buenos_Aires", "ART3"},
    {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
    {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
    {"America/Guatemala", "CST6"},
    {"America/Halifax", "AST4ADT,M3.2.0,M11.1.0"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
    {"America/Mexico_City", "CST6CDT,M4.1.0,M10.5.0"},
    {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Phoenix", "MST7"},
    {"America/Santiago", "CLT4CLST,M9.1.6/24,M4.1.6/24"},
    {"America/Sao_Paulo", "BRT3"},
    {"America/St_Johns", "NST3:30NDT,M3.2.0,M11.1.0"},
    {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},
    {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},
    {"Asia/Almaty", "ALMT-6"},
    {"Asia/Amman", "EET-2EEST,M3.5.4/24,M10.5.5/1"},
    {"Asia/Baghdad", "AST-3"},
    {"Asia/Bangkok", "ICT-7"},
    {"Asia/Beirut", "EET-2EEST,M3.5.0/0,M10.5.0/0"},
    {"Asia/Dhaka", "BDT-6"},
    {"Asia/Dubai", "GST-4"},
    {"Asia/Ho_Chi_Minh", "ICT-7"},
    {"Asia/Hong_Kong", "HKT-8"},
    {"Asia/Jakarta", "WIB-7"},
    {"Asia/Jerusalem", "IST-2IDT,M3.4.4/26,M10.5.0"},
    {"Asia/Karachi", "PKT-5"},
    {"Asia/Kathmandu", "NPT-5:45"},
    {"Asia/Kolkata", "IST-5:30"},
    {"Asia/Kuala_Lumpur", "MYT-8"},
    {"Asia/Manila", "PST-8"},
    {"Asia/Seoul", "KST-9"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Singapore", "SGT-8"},
    {"Asia/Taipei", "CST-8"},
    {"Asia/Tashkent", "UZT-5"},
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Ulaanbaatar", "ULAT-8"},
    {"Asia/Yekaterinburg", "YEKT-5"},
    {"Atlantic/Azores", "AZOT1AZOST,M3.5.0/0,M10.5.0/0"},
    {"Atlantic/Canary", "WET0WEST,M3.5.0/1,M10.5.0"},
    {"Atlantic/Reykjavik", "GMT0"},
    {"Australia/Adelaide", "ACST-9:30ACDT,M10.1.0,M4.1.0"},
    {"Australia/Brisbane", "AEST-10"},
    {"Australia/Darwin", "ACST-9:30"},
    {"Australia/Hobart", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Australia/Perth", "AWST-8"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0"},
    {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Belgrade", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Bucharest", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Dublin", "GMT0IST,M3.5.0/1,M10.5.0"},
    {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Istanbul", "TRT-3"},
    {"Europe/Kiev", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"Europe/Lisbon", "WET0WEST,M3.5.0/0,M10.5.0/0"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Moscow", "MSK-3"},
    {"Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0"},
    {"Pacific/Chatham", "CHAST-12:45CHADT,M9.5.0,M4.1.0/3"},
    {"Pacific/Fiji", "FJT-12"},
    {"Pacific/Guam", "ChST-10"},
    {"Pacific/Honolulu", "HST10"},
    {"Pacific/Port_Moresby", "PGT-10"},
    {"Pacific/Tahiti", "TAHT10"},
    {"UTC", "UTC0"},
    {"Etc/GMT+1", "GMT-1"},
    {"Etc/GMT-1", "GMT+1"}
};

#define TZ_MAPPINGS_COUNT (sizeof(tz_mappings)/sizeof(tz_mappings[0]))

inline const char* ianaToPosix(const char* iana) {
    for (size_t i = 0; i < TZ_MAPPINGS_COUNT; i++) {
        if (strcmp(iana, tz_mappings[i].iana) == 0)
            return tz_mappings[i].posix;
    }
    return "UTC0"; // fallback
}

#endif // TZ_LOOKUP_H