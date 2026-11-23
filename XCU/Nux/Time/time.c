#include <nux/time.h>
#define NUX_TIMER_LIMIT 128u
#define NUX_TIME_THREAD_LIMIT 512u
extern void nux_arch_lock(volatile nux_u32 *lock);
extern void nux_arch_unlock(volatile nux_u32 *lock);
extern nux_status nux_arch_clock_init(void);
extern nux_u64 nux_arch_clock_frequency(void);
extern nux_u64 nux_arch_clock_ticks(void);
typedef struct {nux_timer_id id;nux_clock_id clock;nux_timer_info info;} timer_entry;
typedef struct {nux_u32 id;nux_u64 runtime;} thread_time_entry;
static timer_entry timers[NUX_TIMER_LIMIT];
static thread_time_entry usage[NUX_TIME_THREAD_LIMIT];
static volatile nux_u32 time_lock;
static nux_u64 next_timer_id,frequency;
static nux_u64 ticks_to_ns(nux_u64 ticks) {
    if(!frequency)return 0;
    return (ticks/frequency)*1000000000ULL+((ticks%frequency)*1000000000ULL)/frequency;
}
nux_status nux_time_system_init(void) {
    nux_arch_lock(&time_lock);
    for(nux_u32 i=0;i<NUX_TIMER_LIMIT;i++)timers[i].info.active=0;
    for(nux_u32 i=0;i<NUX_TIME_THREAD_LIMIT;i++){usage[i].id=0;usage[i].runtime=0;}
    next_timer_id=1;frequency=0;
    nux_arch_unlock(&time_lock);
    nux_status st=nux_arch_clock_init();if(st!=NUX_OK)return st;
    frequency=nux_arch_clock_frequency();return frequency?NUX_OK:NUX_E_NOT_SUPPORTED;
}
nux_u64 nux_clock_now(nux_clock_id clock) {
    if(clock!=NUX_CLOCK_MONOTONIC&&clock!=NUX_CLOCK_BOOT)return 0;
    return ticks_to_ns(nux_arch_clock_ticks());
}
nux_status nux_clock_get_resolution(nux_clock_id clock,nux_u64 *out) {
    if(!out||(clock!=NUX_CLOCK_MONOTONIC&&clock!=NUX_CLOCK_BOOT))return NUX_E_INVALID;
    if(!frequency)return NUX_E_BAD_STATE;
    *out=(1000000000ULL+frequency-1)/frequency;return NUX_OK;
}
static timer_entry *timer_find(nux_timer_id id) {
    for(nux_u32 i=0;i<NUX_TIMER_LIMIT;i++)if(timers[i].info.active&&timers[i].id==id)return &timers[i];
    return NUX_NULL;
}
nux_status nux_timer_create(nux_clock_id clock,nux_timer_id *out) {
    if(!out||(clock!=NUX_CLOCK_MONOTONIC&&clock!=NUX_CLOCK_BOOT))return NUX_E_INVALID;
    nux_arch_lock(&time_lock);
    for(nux_u32 i=0;i<NUX_TIMER_LIMIT;i++)if(!timers[i].info.active){
        timer_entry *t=&timers[i];t->id=next_timer_id++;if(!t->id)t->id=next_timer_id++;
        t->clock=clock;t->info.deadline_ns=0;t->info.interval_ns=0;t->info.expirations=0;t->info.armed=0;t->info.active=1;*out=t->id;
        nux_arch_unlock(&time_lock);return NUX_OK;
    }
    nux_arch_unlock(&time_lock);return NUX_E_LIMIT;
}
nux_status nux_timer_arm(nux_timer_id id,nux_u64 deadline,nux_u64 interval) {
    nux_arch_lock(&time_lock);timer_entry *t=timer_find(id);
    if(!t){nux_arch_unlock(&time_lock);return NUX_E_NOT_FOUND;}
    if(!deadline){nux_arch_unlock(&time_lock);return NUX_E_INVALID;}
    t->info.deadline_ns=deadline;t->info.interval_ns=interval;t->info.armed=1;t->info.expirations=0;
    nux_arch_unlock(&time_lock);return NUX_OK;
}
nux_status nux_timer_cancel(nux_timer_id id) {
    nux_arch_lock(&time_lock);timer_entry *t=timer_find(id);
    if(!t){nux_arch_unlock(&time_lock);return NUX_E_NOT_FOUND;}
    t->info.armed=0;nux_arch_unlock(&time_lock);return NUX_OK;
}
nux_status nux_timer_get_info(nux_timer_id id,nux_timer_info *out) {
    if(!out)return NUX_E_INVALID;
    nux_arch_lock(&time_lock);
    timer_entry *t=timer_find(id);
    if(!t){nux_arch_unlock(&time_lock);return NUX_E_NOT_FOUND;}
    *out=t->info;nux_arch_unlock(&time_lock);return NUX_OK;
}
nux_u32 nux_timer_poll(void) {
    nux_u32 expired=0;
    nux_arch_lock(&time_lock);
    for(nux_u32 i=0;i<NUX_TIMER_LIMIT;i++){
        timer_entry *t=&timers[i];if(!t->info.active||!t->info.armed)continue;
        nux_u64 now=nux_clock_now(t->clock);if(now<t->info.deadline_ns)continue;
        ++t->info.expirations;++expired;
        if(t->info.interval_ns){nux_u64 late=now-t->info.deadline_ns; nux_u64 steps=late/t->info.interval_ns+1;
            if(steps>(NUX_U64_MAX-t->info.deadline_ns)/t->info.interval_ns)t->info.armed=0;
            else t->info.deadline_ns+=steps*t->info.interval_ns;}
        else t->info.armed=0;
    }
    nux_arch_unlock(&time_lock);return expired;
}
nux_status nux_thread_sleep_until(nux_u64 deadline) {
    if(!deadline)return NUX_E_INVALID;
    return nux_clock_now(NUX_CLOCK_MONOTONIC)>=deadline?NUX_OK:NUX_E_WOULD_BLOCK;
}
nux_status nux_thread_sleep_for(nux_u64 duration) {
    if(!duration)return NUX_OK;
    nux_u64 now=nux_clock_now(NUX_CLOCK_MONOTONIC);
    if(!now||duration>NUX_U64_MAX-now)return NUX_E_NOT_SUPPORTED;
    return nux_thread_sleep_until(now+duration);
}
nux_status nux_time_account_thread(nux_u32 id,nux_u64 elapsed) {
    if(!id)return NUX_E_INVALID;
    nux_arch_lock(&time_lock);
    for(nux_u32 i=0;i<NUX_TIME_THREAD_LIMIT;i++)if(usage[i].id==id||usage[i].id==0){
        if(!usage[i].id)usage[i].id=id;
        if(elapsed>NUX_U64_MAX-usage[i].runtime){nux_arch_unlock(&time_lock);return NUX_E_LIMIT;}
        usage[i].runtime+=elapsed;nux_arch_unlock(&time_lock);return NUX_OK;
    }
    nux_arch_unlock(&time_lock);return NUX_E_LIMIT;
}
nux_u64 nux_time_thread_runtime(nux_u32 id) {
    if(!id)return 0;
    nux_arch_lock(&time_lock);
    for(nux_u32 i=0;i<NUX_TIME_THREAD_LIMIT;i++)if(usage[i].id==id){nux_u64 r=usage[i].runtime;nux_arch_unlock(&time_lock);return r;}
    nux_arch_unlock(&time_lock);return 0;
}
