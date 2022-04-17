#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <vector>
#include "vma.hpp"
#include "pte.hpp"

#define MAX_VPAGES 64

class Process {
private:
    int pid;
    std::vector<VMA*> vmas;
    pte_t* page_table;

public:
    // stats
    unsigned long unmaps, maps, ins, outs, fins, fouts, zeros, segv, segprot;

    Process(int vma_count, int idx);
    std::vector<VMA*>& getVmas();
    VMA* getVma(int idx);
    pte_t* getPageTable();
    pte_t* getPte(int idx);
    int getId();
    void setVma(int idx, VMA* vma);
    bool canAccessPage(int vpage);

    ~Process();
};

#endif