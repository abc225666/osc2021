#define CORE0_TIMER_IRQ_CTRL 0x40000040

void core_timer_enable();
unsigned long get_boot_time();
void set_time_interval(unsigned long interval);
