#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include <string>
#include <vector>
#include "DiskMultiMap.h"
#include <fstream>
#include <queue>

class IntelWeb
{
public:
    IntelWeb();
    ~IntelWeb();
    
    bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
    
    bool openExisting(const std::string& filePrefix);
    
    void close();
    
    bool ingest(const std::string& telemetryFile);
    
    unsigned int crawl(const std::vector<std::string>& indicators,
                       unsigned int minPrevalenceToBeGood,
                       std::vector<std::string>& badEntitiesFound,
                       std::vector<InteractionTuple>& badInteractions
                       );
    
    bool purge(const std::string& entity);
    
private:
    DiskMultiMap DMM;
    DiskMultiMap DMM_reverse;
    
    fstream inputFile;
    
    int m_numBuckets;
    
    std::string m_filePrefix;
    
    struct checkItem{
        
        checkItem(std::string in_st): st(in_st){}
        
        std::string st;
        int count = 1;
    };
};

#endif // INTELWEB_H_
