#ifndef VMA_HPP
#define VMA_HPP

class VMA {
private:
    int start_vpage, end_vpage;
    bool write_protected, file_mapped;

public:
    VMA() = default;
    VMA(int s, int e, bool w, bool f);
    int getStartPage();
    int getEndPage();
    bool isWriteProtected();
    bool isFileMapped();
    void setF(bool f);
    void setW(bool w);
};

#endif
