/* gpmc_pulse.h */
#pragma once

#include <linux/types.h>
#include <linux/ioctl.h>

#define EXP_PULSE_IOC_MAGIC  'p'

/*
 * Generic GPMC pulse request.
 * The driver will pulse at: (allow_base + offset) unless phys_base != 0.
 * If phys_base != 0, it must still lie within an allowed range.
 *   offset=0x0, width_ns=1000, access_width=1, hi_val=0x80, lo_val=0x00
 */
struct exp_pulse_req {
    __u64 phys_base;      /* optional. 0 => use DT allow-base */
    __u32 offset;         /* byte offset from phys_base */
    __u32 width_ns;       /* pulse width in nanoseconds */

    __u32 access_width;   /* bytes: 1,2,4 (start with 1) */
    __u32 flags;          /* reserved; must be 0 */

    __u32 hi_val;         /* value written for "high" */
    __u32 lo_val;         /* value written for "low" */
};

#define EXP_PULSE_IOC_DO  _IOW(EXP_PULSE_IOC_MAGIC, 0x01, struct exp_pulse_req)
