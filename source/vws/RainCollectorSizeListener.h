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
#ifndef RAIN_COLLECTOR_SIZE_LISTENER_H_
#define RAIN_COLLECTOR_SIZE_LISTENER_H_

#include "WeatherTypes.h"

namespace vws {

/**
 * Interface class used to get notifications when the rain collector size changes.
 */
class RainCollectorSizeListener {
public:
    /**
     * Destructor.
     */
    virtual ~RainCollectorSizeListener() {}

    /**
     * Called when the rain collector size changes or at startup.
     * It does not appear that .1 mm is a valid option as there are no parts sold with a .1 mm capacity.
     *
     * @param bucketSize The size of the rain bucket in inches (.01 inches, .1 mm, or .2 mm (.0079 inches))
     */
    virtual void processRainCollectorSizeChange(Rainfall bucketSize) = 0;
};

}
#endif
