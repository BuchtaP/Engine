//developer options
#define enable_debug

//modules
#define enable_physics
#define enable_audio

//rendering
#define enable_LOD
#define enable_camera_frustum

//world
#define enable_terrain_chunks
#define enable_world_tools
#define enable_weather_system
#define enable_day_night_cycle

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
      


