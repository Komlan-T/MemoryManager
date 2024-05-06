#include "MemoryManager.h" 
#include <vector>
#include <math.h>
#include <list>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

Node::Node(int _length, int start, void *ptr) :
    length(_length),
    free(true),
    offset(start),
    addr(ptr)
{}
MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator) :
    _wordSize(wordSize),
    aLLocator(allocator),
    initialized(false),
    memArray(nullptr),
    memArraySize(0),
    numHoles(0)
{}
MemoryManager::~MemoryManager(){
    shutdown();
}
void MemoryManager::initialize(size_t sizeInWords){
    if(memArray != nullptr){
        shutdown();
    }
    if(sizeInWords <= 65536){
        memArraySize = sizeInWords * _wordSize;
        memArray = new uint8_t[memArraySize];
        Node initialHole(sizeInWords, 0, nullptr);
        vec.push_back(initialHole);
        numHoles = 1;
        initialized = true;
    }
}
void MemoryManager::shutdown(){
    if(!initialized){
        return;
    }
    delete[] memArray;
    memArray = nullptr;
    vec.clear();
    numHoles = 0;
    memArraySize = 0;
    initialized = false;
}
void *MemoryManager::allocate(size_t sizeInBytes){
    if(!initialized){
        return nullptr;
    }
    int words = sizeInBytes / _wordSize;
    if(sizeInBytes % _wordSize != 0){
        words++;
    }
    void *list = getList();
    static_cast<uint16_t*>(list)[0] = numHoles;
    int index = aLLocator(words, list);
    delete[] static_cast<uint16_t*>(list);    
    if(index < 0){
        return nullptr;
    }   
    uint8_t *temp = memArray;
    Node block(words, index, temp + index * _wordSize);
    block.free = false;
    for(int i = 0; i < vec.size(); i++){
        if(vec[i].offset == index){
            if(vec[i].length == words){
                vec[i].free = false;
                vec[i].addr = temp + index * _wordSize;
                numHoles--;           
                return temp + index * _wordSize;
            }
            vec[i].length = vec[i].length - words;
            vec[i].offset = vec[i].offset + words;
            vec.insert(vec.begin() + i, block);
            return temp + index * _wordSize;
        }
    }
    return nullptr;
}
void MemoryManager::free(void* address) {
    if (!initialized){
        return;
    }
    bool found = false;
    for(int i = 0; i < vec.size(); i++){
        if(vec[i].addr == address){
            vec[i].free = true;
            vec[i].addr = nullptr;
            numHoles++;
            found = true;
            break;
        }
    }
    if(!found){
        return;
    }
    for(int i = 0; i < vec.size(); i++){
        if(i < vec.size() - 1 && vec[i].free && vec[i + 1].free){
            vec[i].length = vec[i].length + vec[i + 1].length;
            vec.erase(vec.begin() + i + 1);
            numHoles--;
            break;
        }
    }
}
void *MemoryManager::getList(){
    if(!initialized){
        return nullptr;
    }    
    int k = 0;
    vector<uint16_t> temp;
    temp.push_back(numHoles);
    for(int j = 0; j < vec.size(); j++){
        if(vec[j].free){
            temp.push_back(vec[j].offset);
            temp.push_back(vec[j].length);
        }
    }
    uint16_t *arr = new uint16_t[temp.size()];
    for(int i = 0; i < temp.size(); i++){
        arr[i] = temp[i];
    }
    return arr;
}
void MemoryManager::setAllocator(function<int(int, void *)> allocator){
    aLLocator = allocator;
}
int MemoryManager::dumpMemoryMap(char *filename){
    if(!initialized){
        return -1;
    }    
    int fuga = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(fuga == -1){
        return -1;
    }
    int k = 1;
    string holes = "";
    for(int i = 0; i < vec.size(); i++){
        if(vec[i].free){
            holes += " - ";
            holes += "[";
            holes += to_string(vec[i].offset);
            holes += ", ";
            holes += to_string(vec[i].length);
            holes += "]"; 
        }
    }
    holes = holes.substr(3);
    const char *tran = holes.c_str();
    size_t tranSize = holes.size();
    ssize_t send = write(fuga, tran, tranSize);
    close(fuga);
    return 0;
}
void *MemoryManager::getBitmap(){
    if(!initialized){
        return nullptr;
    }    
    vector<int> bits;
    for(int i = 0; i < vec.size(); i++){
        if(vec[i].free){
            for(int j = 0; j < vec[i].length; j++){
                bits.push_back(0);
            }
        } else {
            for(int j = 0; j < vec[i].length; j++){
                bits.push_back(1);
            }
        }
    }
    if(bits.size() % 8 != 0){
        while(bits.size() % 8 != 0){
            bits.push_back(0);
        }
    }
    vector<vector<int>> bytes;
    vector<int> temp;
    for(int i = 0; i < bits.size(); i++){
        temp.push_back(bits[i]);
        if(temp.size() == 8){
            bytes.push_back(temp);
            temp.clear();
        }
    }
    for(int i = 0; i < bytes.size(); i++){
        std::reverse(bytes[i].begin(), bytes[i].end());
    }
    int size = bytes.size();
    vector<int> arrSize;

    while(size > 0){
        arrSize.push_back(size % 2);
        size /= 2;
    }
    if(arrSize.size() % 8 != 0){
        while(arrSize.size() % 8 != 0){
            arrSize.push_back(0);
        }
    }
    std::reverse(arrSize.begin(), arrSize.end());
    if(arrSize.size() == 8){
        vector<int> filler;
        for(int i = 0; i < 8; i++){
            filler.push_back(0);
        }
        bytes.insert(bytes.begin(), filler);
        bytes.insert(bytes.begin(), arrSize);
    }
    if(arrSize.size() == 16){
        vector<int> x;
        vector<int> y;
        for(int i = 0; i < arrSize.size(); i++){
            if(i < 8){
                x.push_back(arrSize[i]);
            } else {
                y.push_back(arrSize[i]);
            }
        }
        bytes.insert(bytes.begin(), y);
        bytes.insert(bytes.begin(), x);
    }
    void* arr = new uint8_t[bytes.size() * 8];
    for(int i = 0; i < bytes.size(); i++){
        uint8_t x = 0;
        for(int j = 0; j < 8; j++){
            x += bytes[i][j] * pow(2, 7 - j);
        }
        static_cast<uint8_t*>(arr)[i] = x;
    }
    return arr;
}
unsigned MemoryManager::getWordSize(){
    return _wordSize;
}
void *MemoryManager::getMemoryStart(){   
    return memArray;
}
unsigned MemoryManager::getMemoryLimit(){
    return memArraySize;
}
int bestFit(int sizeInWords, void *list){
    uint16_t *temp = (uint16_t*)list;
    int bestFitSize = 10000;
    int bestFitStart = -1; 

    for (int i = 1; i <= temp[0] * 2; i += 2) {
        int holeSize = temp[i+1];
        int holeStart = temp[i];
        if (sizeInWords <= holeSize && holeSize < bestFitSize) {
            bestFitSize = holeSize;
            bestFitStart = holeStart;
        }
    }
    return bestFitStart;
}
int worstFit(int sizeInWords, void *list){  
    uint16_t *temp = (uint16_t*)list;
    int worstFitSize = -1;
    int worstFitStart = -1; 

    for (int i = 1; i <= temp[0] * 2; i += 2) {
        int holeSize = temp[i+1];
        int holeStart = temp[i];
        if (sizeInWords <= holeSize && holeSize > worstFitSize) {
            worstFitSize = holeSize;
            worstFitStart = holeStart;
        }
    }
    return worstFitStart;
}