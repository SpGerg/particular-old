#ifndef PTCL_UTILITY_H
#define PTCL_UTILITY_H

#define PTCL_FLAGS_HAS(flags, flag)   (((flags) & (flag)) == (flag))
#define PTCL_FLAGS_SET(flags, flag)   ((flags) |= (flag))
#define PTCL_FLAGS_REMOVE(flags, flag) ((flags) &= ~(flag))
#define PTCL_FLAGS_TOGGLE(flags, flag, value) \
    do { \
        if (value) \
            PTCL_FLAGS_SET(flags, flag); \
        else \
            PTCL_FLAGS_REMOVE(flags, flag); \
    } while(0)


#endif //PTCL_UTILITY_H