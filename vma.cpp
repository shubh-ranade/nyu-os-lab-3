#include "vma.hpp"

VMA::VMA(int s, int e, bool w, bool f)
    : start_vpage(s)
    , end_vpage(e)
    , write_protected(w)
    , file_mapped(f)
{}

int VMA::getStartPage() { return this->start_vpage; }
int VMA::getEndPage() { return this->end_vpage; }
bool VMA::isWriteProtected() { return this->write_protected; }
bool VMA::isFileMapped() { return this->file_mapped; }
void VMA::setW(bool w) { this->write_protected = w; }
void VMA::setF(bool f) { this->file_mapped = f; }
