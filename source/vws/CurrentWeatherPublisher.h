/*
 * Copyright (C) 2025 Bruce Beisel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CURRENT_WEATHER_PUBLISHER_H
#define CURRENT_WEATHER_PUBLISHER_H


namespace vws {
class CurrentWeather;

/**
 * Interface class that will publish current weather JSON.
 */
class CurrentWeatherPublisher {
public:
    /**
     * Virtual destructor.
     */
    virtual ~CurrentWeatherPublisher() {}

    /**
     * Publish a current weather record.
     *
     * @param currentWeather The current weather record to be published
     */
    virtual void publishCurrentWeather(const CurrentWeather & currentWeather) = 0;
};

}

#endif /* CURRENT_WEATHER_PUBLISHER_H */
