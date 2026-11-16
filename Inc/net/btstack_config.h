/* btstack_config.h: BTstack's settings on the Colour Maximite 2 (docs/Bluetooth_Plan.md, step 3). The PicoMite's own
   bluetooth/btstack_config.h, as tools/port_bt.py copied it (Src/net/bt/picomite), for its Bluetooth host build, then
   what the CMM2 changes.

   BTstack's memory pools (the HCI connections with their ACL buffers, the L2CAP channels and services, the HID,
   RFCOMM, AVDTP and GATT clients' records, the SDP records): the PicoMite keeps them static, with a MAX_NR_ count
   each. The CMM2's internal RAM (the DTCM, where the firmware's statics are) has no room for them, so they come from
   malloc instead (HAVE_MALLOC, which the PicoMite's file already sets: btstack_memory.c takes malloc for a pool with
   no count): newlib's arena, which grows up from the bottom of the SDRAM heap and outlives RUN and NEW. The counts
   that size arrays rather than pools (the LE device database, the security manager's lookups, the whitelist) stay. */
#ifndef CMM2_BTSTACK_CONFIG_H
#define CMM2_BTSTACK_CONFIG_H

#ifndef PICOMITEBTH
#define PICOMITEBTH 1                           // (the PicoMite's BT host build: its settings below)
#endif
#include "picomite/btstack_config.h"

#undef MAX_NR_HCI_CONNECTIONS
#undef MAX_NR_L2CAP_CHANNELS
#undef MAX_NR_L2CAP_SERVICES
#undef MAX_NR_HID_HOST_CONNECTIONS
#undef MAX_NR_RFCOMM_MULTIPLEXERS
#undef MAX_NR_RFCOMM_CHANNELS
#undef MAX_NR_RFCOMM_SERVICES
#undef MAX_NR_AVDTP_CONNECTIONS
#undef MAX_NR_AVDTP_STREAM_ENDPOINTS
#undef MAX_NR_GATT_CLIENTS
#undef MAX_NR_HIDS_CLIENTS
#undef MAX_NR_HIDS_HOSTS
#undef MAX_NR_SERVICE_RECORD_ITEMS

#endif
