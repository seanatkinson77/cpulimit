/**
 *
 * cpulimit - a CPU limiter for Linux
 *
 * Copyright (C) 2005-2012, by:  Angelo Marletta <angelo dot marletta at gmail dot com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __BATTERY__

#define __BATTERY__

enum battery_mode {BATTERY_IGNORED = 0, BATTERY_CHARGING, BATTERY_FULL};

struct battery_status {
    enum battery_mode mode; //mode requested on command line
    char pause;             //non-zero if cpu should be paused
    struct timeval latest;  //time of last battery sample (avoid excessive tests)
};

int battery_pause(struct battery_status*);

#endif
