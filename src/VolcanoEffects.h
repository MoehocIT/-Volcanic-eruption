#ifndef VOLCANO_EFFECTS_H
#define VOLCANO_EFFECTS_H

namespace VolcanoEffects
{
    enum WeatherMode { WEATHER_CLEAR=0, WEATHER_ASHFALL=1, WEATHER_RAIN=2 };
    void init();
    void update(float deltaSeconds);
    void draw();
    void setWeatherMode(WeatherMode mode);
    WeatherMode getWeatherMode();
    void release();
}
#endif
