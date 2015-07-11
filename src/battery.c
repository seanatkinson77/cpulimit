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
 *
 **************************************************************
 *
 * This is a simple program to limit the cpu usage of a process
 * If you modify this code, send me a copy please
 *
 * Get the latest version at: http://github.com/opsengine/cpulimit
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "battery.h"

#ifdef __APPLE__
static const unsigned SAMPLE_USECS = 1000 * 1000;

//return t1-t2 in microseconds (no overflow checks, so better watch out!)
static inline unsigned long timediff(const struct timeval *t1,const struct timeval *t2)
{
	return (t1->tv_sec - t2->tv_sec) * 1000000 + (t1->tv_usec - t2->tv_usec);
}

static int wait_timeval(struct timeval *tv)
{
    struct timeval tmp;
    gettimeofday(&tmp, NULL);

    const unsigned long usecs = timediff(&tmp, tv);

    if (usecs > SAMPLE_USECS)
        return 1;

    memcpy(tv, &tmp, sizeof(tmp));
    return 0;
}

int battery_pause(struct battery_status *status)
{
    if (status->mode == BATTERY_IGNORED)
        return 0;
    else if (!wait_timeval(&status->latest)) {
        return status->pause != 0; // skip new sample, use last status
    }

    FILE *stream = popen("ioreg -c AppleSmartBattery -w0", "r");
    if (!stream) {
        perror("popen");
        return 0;
    }

    enum battery_mode mode = BATTERY_IGNORED;
    while (1) {
        //     | |           "ExternalConnected" = Yes
        char key[256], value[4];
        const int n = fscanf(stream, "    | |           \"%255[^\"]\" = %3s", (char*)&key, (char*)&value);
        if (n == EOF) {
            break;
        } else if (n != 2) {
            if (fgets(key, sizeof(key), stream) != key)
                break;
            else
                continue;
        }
        if (!strcmp("No", value)) {
            continue; // ignore negative values
        } else if (strcmp("Yes", value)) {
            continue; // skip unexepected values
        } else if (!strcmp("ExternalConnected", key) && mode != BATTERY_FULL) {
            mode = BATTERY_CHARGING;
        } else if (!strcmp("FullyCharged", key)) {
            mode = BATTERY_FULL;
        } else {
            continue;
        }
    }

    if (pclose(stream)) {
        perror("pclose");
        return 0;
    }

    status->pause = (mode < status->mode);
    return status->pause;
}
#endif // __APPLE__

#ifdef __linux__
int battery_pause(struct battery_status *status)
{
    if (status->mode == BATTERY_IGNORED)
        return 0;

    FILE * f = fopen("/sys/class/power_supply/BAT0/status", "r");
    if (!f)
        return 0;

    enum battery_mode mode = BATTERY_IGNORED;
    {
        char buf[16];
        if (buf != fgets(buf, sizeof(buf), f))
            return 0;
        fclose(f);

        if (!strcmp("Charging\n", buf))
            mode = BATTERY_CHARGING;
        else if (!strcmp("Full\n", buf))
            mode = BATTERY_FULL;
        else if (strcmp("Discharging\n", buf))
            fprintf(stderr, "Ignoring unknown battery status %s", buf); // buf includes new line
    }

    return mode >= status->mode; // e.g. return true when full but charging requested
}
#endif // __linux__

#ifndef BATTERY_SUPPORT
int battery_pause(struct battery_status *ignored)
{
    return 0;
}
#endif
