#include "IntelWeb.h"

IntelWeb::IntelWeb(){
    
}

IntelWeb::~IntelWeb(){
    close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
    int numBuckets = maxDataItems / 0.75;
    m_numBuckets = numBuckets;
    m_filePrefix = filePrefix;
    return DMM.createNew(filePrefix, numBuckets);
}

bool IntelWeb::openExisting(const std::string& filePrefix){
    return DMM.openExisting(filePrefix);
}

void IntelWeb::close(){
    DMM.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile){
    std::string context;
    std::string key;
    std::string value;
    openExisting(m_filePrefix);
    
    
    // file operations to insert data into the on-disk data structure
    inputFile.open(telemetryFile);
    if (inputFile.is_open()){
        
        while (!inputFile.eof()){
            inputFile >> context;
            inputFile >> key;
            inputFile >> value;
            DMM.insert(key, value, context);
        }
        close();
        inputFile.close();
    }
    else{
        close();
        return false;
    }
    
    // second on-disk data structure with keys and values switched, used for detecting malicious entries
    assert(DMM_reverse.createNew(m_filePrefix + "_reverse", m_numBuckets));
    assert(DMM_reverse.openExisting(m_filePrefix + "_reverse"));
    
    inputFile.open(telemetryFile);
    if (inputFile.is_open()){
        
        while (!inputFile.eof()){
            inputFile >> context;
            inputFile >> key;
            inputFile >> value;
            DMM.insert(value, key, context);
        }
        DMM_reverse.close();
        inputFile.close();
    }
    else{
        DMM_reverse.close();
        return false;
    }
    
    return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators,
                   unsigned int minPrevalenceToBeGood,
                   std::vector<std::string>& badEntitiesFound,
                   std::vector<InteractionTuple>& badInteractions
                   )
{
    std::queue<std::string> que;
    std::vector<checkItem> check;
    
    for (int i = 0; i < indicators.size(); i++){
        que.push(indicators[i]);
    }
    
    // while the queue contains malicious items to be scanned, keep running the loop
    while (!que.empty()){
        
        std::string st = que.front();
        que.pop();
        DiskMultiMap::Iterator it = DMM.search(st);
        DiskMultiMap::Iterator it2 = DMM_reverse.search(st);
        
        // check the malicious keys in the first on-disk data structure
        if (it.isValid() && (*it).key == st){
            badEntitiesFound.push_back(st);
            
            InteractionTuple tuple((*it).key, (*it).value, (*it).context);
            badInteractions.push_back(tuple);
            
            checkItem item((*it).value);
            check.push_back(item);
            ++it;
            bool flag = false;
            while (it.isValid()){
                if ((*it).key == st){
                    checkItem item((*it).value);
                    for (int i = 0; i < check.size(); i++){
                        if (check[i].st == item.st){
                            flag = true;
                            break;
                        }
                    }
                    if (!flag){
                        check.push_back(item);
                    }
                    
                }
                ++it;
            }
        }
        
        // check the malicious values in the second on-disk data structure
        if (it2.isValid() && (*it2).key == st){
            badEntitiesFound.push_back(st);
            
            InteractionTuple tuple((*it2).key, (*it2).value, (*it2).context);
            badInteractions.push_back(tuple);
            
            checkItem item((*it2).value);
            check.push_back(item);
            ++it2;
            bool flag = false;
            while (it2.isValid()){
                if ((*it2).key == st){
                    checkItem item((*it2).value);
                    for (int i = 0; i < check.size(); i++){
                        if (check[i].st == item.st){
                            flag = true;
                            break;
                        }
                    }
                    if (!flag){
                        check.push_back(item);
                    }
                    
                }
                ++it2;
            }
        }
        
        // check the items linked to malicious keys and values & push it onto the queue if they are indeed malicious (<prev)
        for (int i = 0; i < check.size(); i++){
            DiskMultiMap::Iterator it = DMM.search(check[i].st);
            DiskMultiMap::Iterator it2 = DMM_reverse.search(check[i].st);
            
            while (it.isValid()){
                if ((*it).key == check[i].st){
                    check[i].count++;
                }
                ++it;
            }
            
            while (it2.isValid()){
                if ((*it2).key == check[i].st){
                    check[i].count++;
                }
                ++it2;
            }
            
            if (check[i].count < minPrevalenceToBeGood){
                std::vector<std::string>::iterator itv = find (badEntitiesFound.begin(), badEntitiesFound.end(), check[i].st);
                if (itv == badEntitiesFound.end()){
                     que.push(check[i].st);
                }
            }
            check.erase(check.begin()+i);
        }
    }
    
    return badEntitiesFound.size();
}

bool IntelWeb::purge(const std::string& entity)
{
    return true;
}

