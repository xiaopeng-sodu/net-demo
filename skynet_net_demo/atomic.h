#ifndef atomic_h
#define atomic_h


#define ATOM_CAS(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_CAS_POINTER(ptr, oval, nval)  __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_INC(ptr)  __sync_add_and_fetch(ptr, 1)
#define ATOM_DESC(ptr) __sync_sub_and_fetch(ptr, 1)
#define ATOM_ADD(ptr, n)  __sync_add_and_fetch(ptr, n)
#define ATOM_SUB(ptr, n)  __sync_sub_and_fetch(ptr, n)
#define ATOM_AND(ptr, n)  __sync_and_and_fetch(ptr, n)


#endif