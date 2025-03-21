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

class RainCollectorSizeListener {
public:
    /**
     * Destructor.
     */
    virtual ~RainCollectorSizeListener() {}

    /**
     * Called when the rain collector size changes or at startup.
     *
     * @param bucketSize The size of the rain bucket in inches (.01 inches, .02 include, or .01 mm)
     */
    virtual void processRainCollectorSizeChange(Rainfall bucketSize) = 0;
};

}
#endif
