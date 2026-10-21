/* Bounded newlib heap. libnosys' default _sbrk has no stack collision check. */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
extern char __HeapBase, __HeapLimit;
void *_sbrk(ptrdiff_t increment) {
    static uintptr_t current;
    const uintptr_t base=(uintptr_t)&__HeapBase, limit=(uintptr_t)&__HeapLimit;
    if (!current) current=base;
    if ((increment>=0 && (uintptr_t)increment>limit-current) ||
        (increment<0 && (uintptr_t)(-(increment+1))+1>current-base)) {
        errno=ENOMEM; return (void *)-1;
    }
    uintptr_t previous=current;
    current+=increment;
    return (void *)previous;
}
