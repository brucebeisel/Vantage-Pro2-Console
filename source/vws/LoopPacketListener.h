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

#ifndef LOOP_PACKET_LISTENER_H_
#define LOOP_PACKET_LISTENER_H_

namespace vws {
class LoopPacket;
class Loop2Packet;

/**
 * Pure virtual class that listens for loop packets that are received in the loop packet loop.
 */
class LoopPacketListener {
public:
    /**
     * Virtual destructor.
     */
    virtual ~LoopPacketListener() {}

    /**
     * Method that will be called when a LOOP packet is received from the console.
     *
     * @param packet The LOOP packet
     * @return True if the loop packet loop should continue
     */
    virtual bool processLoopPacket(const LoopPacket & packet) = 0;

    /**
     * Method that will be called when a LOOP2 packet is received from the console.
     *
     * @param packet The LOOP2 packet
     * @return True if the loop packet loop should continue
     */
    virtual bool processLoop2Packet(const Loop2Packet & packet) = 0;
};

}

#endif
