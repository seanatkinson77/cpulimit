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
    // fprintf(stderr, "wait_timeval() %lu\n", usecs);

    if (usecs > SAMPLE_USECS)
        return 1;

    memcpy(tv, &tmp, sizeof(tmp));
    return 0;
}

static char parse_ioreg(const char *phrase, const char *pause)
{
    FILE *stream = popen("ioreg -c AppleSmartBattery -w0", "r");
    if (!stream) {
        perror("popen");
        return 0;
    }

    // unsigned reads = 0;
    unsigned yes = 0, no = 0;
    while (1) {
        char value[4];
        {
            char key[32];
            const int n = fscanf(stream, "\"%31[^\"]\" = %3s", (char*)&key, (char*)&value);
            // ++reads;
            // fprintf(stderr, "parse_ioreg() fscanf %i %u\n", n, reads);
            if (n == EOF) {
                // fprintf(stderr, "parse_ioreg() EOF\n");
                break;
            } else if (n != 2) {
                // fprintf(stderr, "Unexpected match count %i for ioreg\n", n);
                fgetc(stream);
                continue;
            } else if (strcmp(phrase, key)) {
                fgetc(stream);
                continue;
            }
        }

        // fprintf(stderr, "parse_ioreg() '%s' '%s'\n", phrase, value);

        if (!strcmp("Yes", value)) {
            ++yes;
        } else if (!strcmp("No", value)) {
            no += 1;
        } else {
            fprintf(stderr, "Unexpected value '%s' from ioreg for '%s'\n", value, phrase);
            continue;
        }
        // fprintf(stderr, "A yes %u, no %u %p\n", yes, blah, &blah);
    }

    // fprintf(stderr, "B yes %u, no %u %p\n", yes, blah, &blah);

    if (pclose(stream)) {
        perror("pclose");
        return 0;
    }

    if (yes + no != 1) {
        fprintf(stderr, "Ignoring unexpected counts from ioreg for '%s': %u yes, %u no\n", phrase, yes, no);
        return 0;
    } else if (yes) {
        if (*pause)
            fprintf(stderr, "Detected battery status change\n");
        return 0;
    } // no == 1, yes == 0

    // fprintf(stderr, "parse_ioreg() sleeping\n");
    if (!*pause) {
        fprintf(stderr, "Waiting for battery status change ...\n");
    }
    return 1;
}

int battery_pause(struct battery_status *status)
{
    // fprintf(stderr, "battery_pause() mode %i\n", status->mode);

    const char *name = NULL;
    switch (status->mode) {
    default: // fall-through if mode isn't valid
    case BATTERY_IGNORED:
        return 0;
    case BATTERY_CHARGING:
        name = "ExternalConnected";
        break;
    case BATTERY_FULL:
        name = "FullyCharged";
        break;
    }

    if (!wait_timeval(&status->latest))
        return status->pause != 0; // skip new sample, use last status

    status->pause = parse_ioreg(name, &status->pause);
    return status->pause;
}
#endif

#ifdef __linux__
int battery_pause(struct battery_status *status)
{
    const char *name = NULL;
    switch (status->mode) {
    default: // fall-through if mode isn't valid
    case BATTERY_IGNORED:
        return 0;
    case BATTERY_CHARGING:
        name = "Charging\n"; // FIXME: confirm ...
        break;
    case BATTERY_FULL:
        name = "Full\n";
        break;
    }

    FILE * f = fopen("/sys/class/power_supply/BAT0/status", "r");
    if (!f)
        return 0;

    int pause = 1;
    {
        char buf[16];
        if (buf == fgets(buf, sizeof(buf), f) && !strcmp(name, buf)) {
            pause = 0;
        }
    }
    fclose(f);


    return pause;
}
#endif

#ifndef BATTERY_SUPPORT
int battery_pause(struct battery_status *ignored)
{
    return 0;
}
#endif
