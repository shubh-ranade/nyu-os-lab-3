#ifndef PTE_HPP
#define PTE_HPP

extern unsigned int MAX_FRAMES;

typedef struct
{
    unsigned int valid:1;
    unsigned int referenced:1;
    unsigned int modified:1;
    unsigned int write_protected:1;
    unsigned int paged_out:1;
    unsigned int frame_number:7;
    unsigned int file_mapped:1;
} pte_t;

typedef struct
{
    unsigned int procid, vpage;
    pte_t* pte_ref;
} frame_t;


#endif
