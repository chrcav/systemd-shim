/*
 * Copyright © 2013 Canonical Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the licence, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 */

#include "unit.h"

#include <stdlib.h>

typedef UnitClass PowerUnitClass;
static GType power_unit_get_type (void);

const gchar *power_cmds[] = {
  [POWER_OFF] = "/sbin/poweroff",
  [POWER_REBOOT] = "/sbin/reboot",
  [POWER_SUSPEND] = "/usr/sbin/pm-suspend",
  [POWER_HIBERNATE] = "/usr/sbin/pm-hibernate"
};

typedef struct
{
  Unit parent_instance;
  PowerAction action;
} PowerUnit;

G_DEFINE_TYPE (PowerUnit, power_unit, UNIT_TYPE)

static void
power_unit_start (Unit *unit)
{
  PowerUnit *pu = (PowerUnit *) unit;
  static gint64 last_suspend_time;
  static gboolean in_shutdown;

  /* If we request power off or reboot actions then we should ignore any
   * suspend or hibernate actions that come after this.
   */
  if (pu->action == POWER_OFF || pu->action == POWER_REBOOT)
    {
      in_shutdown = TRUE;
      system (power_cmds[pu->action]);
    }
  else
    {
      if (in_shutdown)
        return;

      /* This is pretty ugly: if we are being asked to perform a suspend
       * or hibernate action within 1 second of the previous one, don't
       * do it.
       *
       * We will be able to do this properly once we forward the
       * timestamp of the event that caused the suspend all the way
       * down.
       */
      if (pu->action == POWER_SUSPEND && last_suspend_time + G_TIME_SPAN_SECOND > g_get_monotonic_time ())
        return;

      system (power_cmds[pu->action]);

      if (pu->action == POWER_SUSPEND)
        last_suspend_time = g_get_monotonic_time ();
    }
}

static void
power_unit_stop (Unit *unit)
{
}

static const gchar *
power_unit_get_state (Unit *unit)
{
  return "static";
}

Unit *
power_unit_new (PowerAction action)
{
  PowerUnit *unit;

  g_return_val_if_fail (action < N_POWER_ACTIONS, NULL);

  unit = g_object_new (power_unit_get_type (), NULL);
  unit->action = action;

  return (Unit *) unit;
}

static void
power_unit_init (PowerUnit *unit)
{
}

static void
power_unit_class_init (UnitClass *class)
{
  class->start = power_unit_start;
  class->stop = power_unit_stop;
  class->get_state = power_unit_get_state;
}
