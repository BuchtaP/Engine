#include "config.h"

#ifdef enable_LOD
        int LOD_distance[3] = { 100, 200, 300 }; // Prahové hodnoty pro jednotlivé úrovně detailu
#endif

#ifdef enable_weather_system
        bool enable_rain = true; // Povolit déšť
        bool enable_rain_stamps = true; // Povolit stopy deště
        bool enable_snow = false; // Povolit sníh
        bool enable_storms = true; // Povolit bouřky
        bool enable_fog = true; // Povolit mlhu
        bool enable_lightning = true; // Povolit blesky
        bool enable_clouds = true; // Povolit mraky
        bool enable_wind = true; // Povolit vítr
       
#endif