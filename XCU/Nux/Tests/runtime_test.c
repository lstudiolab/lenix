extern void *memcpy(void *destination, const void *source, unsigned long size);
extern void *memmove(void *destination, const void *source, unsigned long size);
extern void *memset(void *destination, int value, unsigned long size);
extern int memcmp(const void *left, const void *right, unsigned long size);

int main(void)
{
    char source[] = "kernel";
    char buffer[16];
    char overlap[] = "abcdefgh";

    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, source, sizeof(source));
    if (memcmp(buffer, "kernel", 7) != 0) return 1;

    memmove(overlap + 2, overlap, 6);
    if (memcmp(overlap, "ababcdef", 8) != 0) return 2;

    memmove(overlap, overlap + 2, 6);
    if (memcmp(overlap, "abcdef", 6) != 0) return 3;

    memset(buffer, 0xa5, sizeof(buffer));
    for (unsigned long i = 0; i < sizeof(buffer); ++i)
        if ((unsigned char)buffer[i] != 0xa5) return 4;

    if (memcmp("a", "b", 1) >= 0 || memcmp("b", "a", 1) <= 0)
        return 5;
    return 0;
}
