#include <nux/user.h>
#include <nux/elf.h>
#include <nux/task.h>
#include <nux/thread.h>
#include <nux/syscall.h>
#include <nux/vm.h>

#define USER_IMAGE_SIZE 8192u
#define USER_ENTRY_ADDRESS 0x40000000ULL
#define USER_SEGMENT_OFFSET 4096u

static nux_u8 user_image[USER_IMAGE_SIZE];
static volatile nux_u32 user_smoke_worker_ran;

static void user_smoke_worker(void *argument)
{
    (void)argument;
    user_smoke_worker_ran = 1;
}

static void put16(nux_u8 *p, nux_u16 value)
{
    p[0] = (nux_u8)value;
    p[1] = (nux_u8)(value >> 8);
}
static void put32(nux_u8 *p, nux_u32 value)
{
    for (nux_u32 i = 0; i < 4; ++i) p[i] = (nux_u8)(value >> (i * 8));
}
static void put64(nux_u8 *p, nux_u64 value)
{
    for (nux_u32 i = 0; i < 8; ++i) p[i] = (nux_u8)(value >> (i * 8));
}

static void build_user_exit_image(void)
{
    for (nux_u32 i = 0; i < USER_IMAGE_SIZE; ++i) user_image[i] = 0;
    user_image[0] = 0x7f;
    user_image[1] = 'E';
    user_image[2] = 'L';
    user_image[3] = 'F';
    user_image[4] = 2;
    user_image[5] = 1;
    user_image[6] = 1;
    put16(user_image + 16, 2);
#if defined(__x86_64__)
    put16(user_image + 18, 62);
    static const nux_u8 code[] = {
        0xb8, 0x00, 0x00, 0x00, 0x00,
        0xbf, 0x2a, 0x00, 0x00, 0x00,
        0xcd, 0x80
    };
#elif defined(__aarch64__)
    put16(user_image + 18, 183);
    static const nux_u8 code[] = {
        0x08, 0x00, 0x80, 0xd2,
        0x40, 0x05, 0x80, 0xd2,
        0x01, 0x00, 0x00, 0xd4
    };
#else
#error Unsupported Nux user ABI
#endif
    put32(user_image + 20, 1);
    put64(user_image + 24, USER_ENTRY_ADDRESS);
    put64(user_image + 32, 64);
    put16(user_image + 52, 64);
    put16(user_image + 54, 56);
    put16(user_image + 56, 1);

    nux_u8 *program = user_image + 64;
    put32(program, 1);
    put32(program + 4, 5);
    put64(program + 8, USER_SEGMENT_OFFSET);
    put64(program + 16, USER_ENTRY_ADDRESS);
    put64(program + 32, 12);
    put64(program + 40, NUX_VM_PAGE_SIZE);
    put64(program + 48, NUX_VM_PAGE_SIZE);
    for (nux_u32 i = 0; i < 12; ++i)
        user_image[USER_SEGMENT_OFFSET + i] = code[i];
}

nux_status nux_user_smoke_test(void)
{
    nux_task_id task = 0;
    nux_thread_id thread = 0, worker = 0;
    nux_thread_info info, worker_info;
    nux_elf_load_result image;
    nux_status status;

    user_smoke_worker_ran=0;
    build_user_exit_image();
    status = nux_task_create("Nux user-mode smoke", 10, &task);
    if (status != NUX_OK) return status;
    status = nux_thread_create_kernel(task, user_smoke_worker, NUX_NULL,
                                       10, &worker);
    if (status != NUX_OK) goto failed;
    status = nux_elf_load(task, user_image, sizeof(user_image), &image);
    if (status != NUX_OK) goto failed;
    status = nux_thread_create_user(task, image.entry, image.stack_pointer,
                                    0, 10, &thread);
    if (status != NUX_OK) goto failed;
    status = nux_thread_yield();
    if (status != NUX_OK) goto failed;
    status = nux_thread_get_info(thread, &info);
    if (status != NUX_OK || info.state != NUX_THREAD_TERMINATED ||
        nux_thread_get_info(worker, &worker_info) != NUX_OK ||
        worker_info.state != NUX_THREAD_TERMINATED || !user_smoke_worker_ran ||
        nux_last_user_exit_code != 42) {
        status = NUX_E_BAD_STATE;
        goto failed;
    }
    return nux_task_destroy(task);

failed:
    if (thread) (void)nux_thread_terminate(thread);
    if (worker) (void)nux_thread_terminate(worker);
    if (task) (void)nux_task_destroy(task);
    return status;
}
