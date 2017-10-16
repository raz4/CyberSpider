#include <iostream>
#include "DiskMultiMap.h"
#include "IntelWeb.h"
#include "InteractionTuple.h"
#include <fstream>

void fillIndicators(std::vector<std::string>& indicators, std::string filename){
    fstream file;
    file.open(filename);
    if (file.is_open()){
        while (!file.eof()){
            std::string context;
            std::string key;
            std::string value;
            file >> context;
            file >> key;
            indicators.push_back(key);
            file >> value;
            indicators.push_back(value);
        }
    }
}


int main() {
    
    /*DiskMultiMap dmm;
    
    dmm.createNew("test", 10);
    dmm.insert("key1", "value1", "context1");
    dmm.insert("key1", "value2", "context2");
    
    dmm.erase("key1", "value2", "context2");
    
    DiskMultiMap::Iterator it = dmm.search("key1");
    
    cout << (*it).value << endl;*/
    
    
    IntelWeb web;
    web.createNew("file", 10000);
    web.ingest("outputlog.txt");
    
    std::vector<std::string> indicators;
    std::vector<std::string> badEntitiesFound;
    std::vector<InteractionTuple> badInteractions;
    fillIndicators(indicators, "malicious.txt");
    
    cout << web.crawl(indicators, 6, badEntitiesFound, badInteractions) << endl;
    
    
    return 0;
}
