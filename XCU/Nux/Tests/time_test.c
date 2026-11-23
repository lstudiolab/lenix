#include <nux/time.h>
static nux_u64 ticks;
static nux_u64 freq=1000000000ULL;
void nux_arch_lock(volatile nux_u32 *lock){while(*lock){}*lock=1;}
void nux_arch_unlock(volatile nux_u32 *lock){*lock=0;}
nux_status nux_arch_clock_init(void){return NUX_OK;}
nux_u64 nux_arch_clock_frequency(void){return freq;}
nux_u64 nux_arch_clock_ticks(void){return ticks;}
int main(void){
    nux_timer_id timer; nux_timer_info info; nux_u64 resolution;
    if(nux_time_system_init()!=NUX_OK)return 1;
    if(nux_clock_get_resolution(NUX_CLOCK_MONOTONIC,&resolution)!=NUX_OK||resolution!=1)return 2;
    if(nux_clock_now(NUX_CLOCK_REALTIME)!=0)return 3;
    if(nux_timer_create(NUX_CLOCK_MONOTONIC,&timer)!=NUX_OK)return 4;
    if(nux_timer_arm(timer,100,10)!=NUX_OK)return 5;
    ticks=99;if(nux_timer_poll()!=0)return 6;
    ticks=100;if(nux_timer_poll()!=1)return 7;
    if(nux_timer_get_info(timer,&info)!=NUX_OK||info.expirations!=1||!info.armed)return 8;
    ticks=109;if(nux_timer_poll()!=0)return 9;
    ticks=110;if(nux_timer_poll()!=1)return 10;
    if(nux_timer_cancel(timer)!=NUX_OK)return 11;
    if(nux_timer_get_info(timer,&info)!=NUX_OK||info.armed)return 12;
    if(nux_thread_sleep_until(200)!=NUX_E_WOULD_BLOCK)return 13;
    if(nux_time_account_thread(7,25)!=NUX_OK||nux_time_thread_runtime(7)!=25)return 14;
    if(nux_time_account_thread(7,5)!=NUX_OK||nux_time_thread_runtime(7)!=30)return 15;
    return 0;
}
