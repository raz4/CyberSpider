#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"
#include <iostream>
#include <cassert>

class DiskMultiMap
{
public:
    
    class Iterator
    {
    public:
        Iterator() : m_valid(false)
        {}
        
        Iterator(int ptr_in, std::string in_name, std::string in_key);
        
        Iterator(const Iterator& rhs);
        
        bool isValid() const;
        
        Iterator& operator++();
        
        MultiMapTuple operator*();
        
    private:
        bool m_valid;
        int ptr;
        std::string m2_filename;
        BinaryFile m_file;
        std::string m_key;
    };
    
    DiskMultiMap();
    
    ~DiskMultiMap();
    
    bool createNew(const std::string& filename, unsigned int numBuckets);
    
    bool openExisting(const std::string& filename);
    
    void close();
    
    bool insert(const std::string& key, const std::string& value, const std::string& context);
    
    Iterator search(const std::string& key);
    
    int erase(const std::string& key, const std::string& value, const std::string& context);
    
private:
    BinaryFile m_file;
    
    int bucketCount;
    
    std::hash<std::string> hashFn;
    
    std::string m_filename;
    
    struct Bucket{
        int ptr;
    };
    
    struct DiskNode{
        
        DiskNode(BinaryFile::Offset input_next) : next(input_next)
        {
        }
        
        DiskNode(){}
        
        DiskNode(std::string in_key, std::string in_value, std::string in_context){
            for (int i = 0; i < in_key.size(); i++){
                key[i] = in_key.c_str()[i];
            }
            key[in_key.size()] = '\0';
            
            for (int i = 0; i < in_value.size(); i++){
                value[i] = in_value.c_str()[i];
            }
            value[in_value.size()] = '\0';
            
            for (int i = 0; i < in_context.size(); i++){
                context[i] = in_context.c_str()[i];
            }
            context[in_context.size()] = '\0';
        }
        
        char key[120 + 1];
        char value[120 + 1];
        char context[120 + 1];
        
        BinaryFile::Offset next = 0;
        
    };
    
    int prevPtrWhenSearching = 0;
    
    struct deletedBucket{
        int ptr = 0;
    };
    
};

#endif // DISKMULTIMAP_H_
