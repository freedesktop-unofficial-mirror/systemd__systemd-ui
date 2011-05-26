/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

#ifndef foologindseathfoo
#define foologindseathfoo

/***
  This file is part of systemd.

  Copyright 2011 Lennart Poettering

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

typedef struct Seat Seat;

#include "list.h"
#include "util.h"
#include "logind.h"
#include "logind-device.h"
#include "logind-session.h"

struct Seat {
        Manager *manager;
        char *id;

        char *state_file;

        LIST_HEAD(Device, devices);

        Session *active;
        LIST_HEAD(Session, sessions);

        bool in_gc_queue:1;

        LIST_FIELDS(Seat, gc_queue);
};

Seat *seat_new(Manager *m, const char *id);
void seat_free(Seat *s);

int seat_save(Seat *s);
int seat_load(Seat *s);

int seat_apply_acls(Seat *s, Session *old_active);
int seat_active_vt_changed(Seat *s, int vtnr);
int seat_read_active_vt(Seat *s);

int seat_start(Seat *s);
int seat_stop(Seat *s);

int seat_check_gc(Seat *s);
void seat_add_to_gc_queue(Seat *s);

#endif