/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <errno.h>
#include <string.h>
#include <libudev.h>

#include "util.h"
#include "sysfs-show.h"

static bool device_has_tag(struct udev_device *d, const char *tag) {
        struct udev_list_entry *first, *item;

        assert(d);
        assert(tag);

        /* FIXME */
        udev_device_get_is_initialized(d);

        first = udev_device_get_tags_list_entry(d);
        udev_list_entry_foreach(item, first)
                if (streq(udev_list_entry_get_name(item), tag))
                        return true;

        return false;
}

static int show_sysfs_one(
                struct udev *udev,
                const char *seat,
                struct udev_list_entry **item,
                const char *sub,
                const char *prefix,
                unsigned n_columns) {

        assert(udev);
        assert(seat);
        assert(item);
        assert(prefix);

        while (*item) {
                struct udev_list_entry *next, *lookahead;
                struct udev_device *d;
                const char *sn, *id, *name, *sysfs, *subsystem, *sysname;

                sysfs = udev_list_entry_get_name(*item);
                if (!path_startswith(sysfs, sub))
                        return 0;

                d = udev_device_new_from_syspath(udev, sysfs);
                if (!d) {
                        *item = udev_list_entry_get_next(*item);
                        continue;
                }

                sn = udev_device_get_property_value(d, "ID_SEAT");
                if (isempty(sn))
                        sn = "seat0";

                /* fixme, also check for tag 'seat' here */
                if (!streq(seat, sn) || !device_has_tag(d, "seat")) {
                        udev_device_unref(d);
                        *item = udev_list_entry_get_next(*item);
                        continue;
                }

                id = udev_device_get_property_value(d, "ID_FOR_SEAT");
                name = udev_device_get_sysattr_value(d, "name");
                if (!name)
                        name = udev_device_get_sysattr_value(d, "id");
                subsystem = udev_device_get_subsystem(d);
                sysname = udev_device_get_sysname(d);

                /* Look if there's more coming after this */
                lookahead = next = udev_list_entry_get_next(*item);
                while (lookahead) {
                        const char *lookahead_sysfs;

                        lookahead_sysfs = udev_list_entry_get_name(lookahead);

                        if (path_startswith(lookahead_sysfs, sub) &&
                            !path_startswith(lookahead_sysfs, sysfs)) {
                                struct udev_device *lookahead_d;

                                lookahead_d = udev_device_new_from_syspath(udev, lookahead_sysfs);
                                if (lookahead_d) {
                                        const char *lookahead_sn;
                                        bool found;

                                        lookahead_sn = udev_device_get_property_value(d, "ID_SEAT");
                                        if (isempty(lookahead_sn))
                                                lookahead_sn = "seat0";

                                        found = streq(seat, lookahead_sn) && device_has_tag(d, "seat");
                                        udev_device_unref(lookahead_d);

                                        if (found)
                                                break;
                                }
                        }

                        lookahead = udev_list_entry_get_next(lookahead);
                }

                printf("%s%s %s (%s:%s)", prefix, lookahead ? "\342\224\234" : "\342\224\224", id ? id : sysfs, subsystem, sysname);

                if (name)
                        printf(" \"%s\"\n", name);
                else
                        printf("\n");

                *item = next;
                if (*item) {
                        char *p;

                        p = strappend(prefix, lookahead ? "\342\224\202 " : "  ");
                        show_sysfs_one(udev, seat, item, sysfs, p, n_columns - 2);
                        free(p);
                }

                udev_device_unref(d);
        }

        return 0;
}

int show_sysfs(const char *seat, const char *prefix, unsigned n_columns) {
        struct udev *udev;
        struct udev_list_entry *first = NULL;
        struct udev_enumerate *e;
        int r;

        if (n_columns <= 0)
                n_columns = columns();

        if (!prefix)
                prefix = "";

        if (isempty(seat))
                seat = "seat0";

        udev = udev_new();
        if (!udev)
                return -ENOMEM;

        e = udev_enumerate_new(udev);
        if (!e) {
                r = -ENOMEM;
                goto finish;
        }

        if (!streq(seat, "seat0"))
                r = udev_enumerate_add_match_tag(e, seat);
        else
                r = udev_enumerate_add_match_tag(e, "seat");

        r = udev_enumerate_scan_devices(e);
        if (r < 0)
                goto finish;

        first = udev_enumerate_get_list_entry(e);
        if (first)
                show_sysfs_one(udev, seat, &first, "/", prefix, n_columns);

finish:
        if (e)
                udev_enumerate_unref(e);

        if (udev)
                udev_unref(udev);

        return r;
}