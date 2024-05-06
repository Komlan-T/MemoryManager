#include <stdio.h>
#include <functional>
#include <iostream>
#include <cstdlib>
#include <list>
#include <vector>

using namespace std;
struct Node{

    int length;
    int offset;
    bool free;
    void *addr;

    Node(int _length, int start, void *ptr);
};
struct MemoryManager{

    unsigned _wordSize;
    function<int(int, void *)> aLLocator;
    bool initialized;
    uint8_t *memArray;
    int memArraySize;
    vector<Node> vec;
    int numHoles;

    MemoryManager(unsigned wordSize, function<int(int, void *)> allocator);
    ~MemoryManager();
    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void *getList();
    void setAllocator(function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();

};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);