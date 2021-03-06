#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/of.h>

#include <mach/hardware.h>
#include <mach/irq.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/bootinfo.h>
#include <asm/div64.h>

#define GOLDFISH_TIMER_LOW		0x00
#define GOLDFISH_TIMER_HIGH		0x04

/*
 * Estimate CPU frequency.  Sets mips_hpt_frequency as a side-effect
 */
static unsigned int __init estimate_cpu_frequency(void)
{
	uint32_t base;
	struct device_node *dn;
	void __iomem *rtc_base;
	unsigned int start, count;
	unsigned long rtcstart, rtcdelta;
	unsigned int prid = read_c0_prid() & 0xffff00;

	if ((dn = of_find_node_by_name(NULL, "goldfish_timer")) == NULL) {
		panic("goldfish_timer_init() failed to "
			  "fetch device node \'goldfish-timer\'!\n");
	}

	if (of_property_read_u32(dn, "reg", &base) < 0) {
		panic("goldfish_timer_init() failed to "
			  "fetch device base address property \'reg\'!\n");
	}

	rtc_base = IO_ADDRESS(base);

	/*
	 * poll the nanosecond resolution RTC for 1s
	 * to calibrate the CPU frequency
	 */
	rtcstart = readl(rtc_base+GOLDFISH_TIMER_LOW);
	start = read_c0_count();
	do {
		rtcdelta = readl(rtc_base+GOLDFISH_TIMER_LOW) - rtcstart;
	} while (rtcdelta < 1000000000);
	count = read_c0_count() - start;

	mips_hpt_frequency = count;
	if ((prid != (PRID_COMP_MIPS | PRID_IMP_20KC)) &&
	    (prid != (PRID_COMP_MIPS | PRID_IMP_25KF)))
		count *= 2;

	count += 5000;    /* round */
	count -= count%10000;

	return count;
}

unsigned long cpu_khz;

void plat_time_init(void)
{
	unsigned int est_freq;

	est_freq = estimate_cpu_frequency();

	printk(KERN_INFO "CPU frequency %d.%02d MHz\n", est_freq/1000000,
	       (est_freq%1000000)*100/1000000);

	cpu_khz = est_freq / 1000;

}

/**
 * save_time_delta - Save the offset between system time and RTC time
 * @delta: pointer to timespec to store delta
 * @rtc: pointer to timespec for current RTC time
 *
 * Return a delta between the system time and the RTC time, such
 * that system time can be restored later with restore_time_delta()
 */
void save_time_delta(struct timespec *delta, struct timespec *rtc)
{
	set_normalized_timespec(delta,
				__current_kernel_time().tv_sec - rtc->tv_sec,
				__current_kernel_time().tv_nsec - rtc->tv_nsec);
}
EXPORT_SYMBOL(save_time_delta);

/**
 * restore_time_delta - Restore the current system time
 * @delta: delta returned by save_time_delta()
 * @rtc: pointer to timespec for current RTC time
 */
void restore_time_delta(struct timespec *delta, struct timespec *rtc)
{
	struct timespec ts;

	set_normalized_timespec(&ts,
				delta->tv_sec + rtc->tv_sec,
				delta->tv_nsec + rtc->tv_nsec);

	do_settimeofday(&ts);
}
EXPORT_SYMBOL(restore_time_delta);
