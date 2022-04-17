#include "process.hpp"
#include<iostream>
#include<string>

using namespace std;

Process::Process(int vma_count, int idx)
    : vmas(vector<VMA*>(vma_count))
    , page_table(new pte_t[MAX_VPAGES]{})
    , pid(idx)
    , maps(0)
    , unmaps(0)
    , ins(0)
    , outs(0)
    , fins(0)
    , fouts(0)
    , zeros(0)
    , segv(0)
    , segprot(0)
{}

vector<VMA*>& Process::getVmas() { return this->vmas; }
VMA* Process::getVma(int idx) { return this->vmas[idx]; }
pte_t* Process::getPageTable() { return this->page_table; }
pte_t* Process::getPte(int idx) { return &this->page_table[idx]; }
int Process::getId() { return this->pid; }
void Process::setVma(int idx, VMA* vma) { this->vmas[idx] = vma; }

// TODO: don't do a linear search every time
bool Process::canAccessPage(int vpage) {
    for(auto vma : this->vmas) {
        if(vpage >= vma->getStartPage() && vpage <= vma->getEndPage()) {
            page_table[vpage].file_mapped = vma->isFileMapped();
            page_table[vpage].write_protected = vma->isWriteProtected();
            return true;
        }
    }
    return false;
}

Process::~Process() {
    for(auto vma : this->vmas)
        delete vma;
    delete [] page_table;
}
