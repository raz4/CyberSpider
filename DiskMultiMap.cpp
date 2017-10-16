#include "DiskMultiMap.h"

DiskMultiMap::DiskMultiMap(){}

DiskMultiMap::~DiskMultiMap(){
    close();
}

DiskMultiMap::Iterator::Iterator(int ptr_in, std::string in_name, std::string in_key)
    : m2_filename(in_name), ptr(ptr_in), m_key(in_key), m_valid(true)
{}

DiskMultiMap::Iterator::Iterator(const DiskMultiMap::Iterator& rhs): m_valid(rhs.m_valid), ptr(rhs.ptr), m2_filename(rhs.m2_filename), m_key(rhs.m_key)
{}

bool DiskMultiMap::Iterator::isValid() const{
    return m_valid;
}

DiskMultiMap::Iterator &DiskMultiMap::Iterator::operator++(){
    if (isValid()){
        assert(m_file.openExisting(m2_filename));
        
        int scan_ptr = ptr;
        DiskNode node;
        assert(m_file.read(node, scan_ptr)); // read node current being pointed to
        
        // keep searching the linked list for nodes with matching keys
        while(node.next != 0){ // check if current node is pointing to another node
            scan_ptr = node.next;
            assert(m_file.read(node, scan_ptr)); // copy in the next node
            
            if (strcmp(node.key, m_key.c_str()) == 0){ // check if the next node's key matches
                ptr = scan_ptr;
                m_file.close();
                return *this;
            }
        }
        
        m_valid = false; // set the state to invalid if there isn't another node (with matching key)
        
        m_file.close();
    }
    
    return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*(){
    MultiMapTuple map;
    assert(m_file.openExisting(m2_filename));
    if (!isValid()){ // check if there this is pointing to a valid node
        map.key = "";
        map.value = "";
        map.context = "";
        return map;
    }
    
    DiskMultiMap::DiskNode node;
    assert(m_file.read(node, ptr)); // read the node
    
    // format and copy the contents of the node into the MultiMapTuple and return it
    char ch = 'a';
    std::string st = "";
    int i = 0;
    while (ch != '\0'){
        ch = node.key[i];
        if (ch != '\0'){
            st += ch;
        }
        i++;
    }
    map.key = st;
    
    ch = 'a';
    i=0;
    st= "";
    while (ch != '\0'){
        ch = node.value[i];
        if (ch != '\0'){
            st += ch;
        }
        i++;
    }
    map.value = st;
    
    ch = 'a';
    i=0;
    st= "";
    while (ch != '\0'){
        ch = node.context[i];
        if (ch != '\0'){
            st += ch;
        }
        i++;
    }
    map.context = st;
    
    m_file.close();
    
    return map;
}

bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets){
    m_file.createNew(filename);
    
    m_filename = filename;
    
    openExisting(filename);
    
    // copy in the number of buckets the hash table will contain
    m_file.write(numBuckets, 0);
    
    // keep track of deleted nodes which are stored in a linked list
    deletedBucket deleteBucket;
    deleteBucket.ptr = 0;
    m_file.write(deleteBucket, m_file.fileLength());
    
    bucketCount = numBuckets;
    
    // copy in the buckets for the hash table
    for (int i = 0; i < numBuckets; i++){
        Bucket bucket;
        bucket.ptr = 0;
        m_file.write(bucket, m_file.fileLength());
        
    }
    close();
    return true;
}

bool DiskMultiMap::openExisting(const std::string& filename){
    return m_file.openExisting(filename);
}

void DiskMultiMap::close(){
    m_file.close();
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context){
    
    if (key.size() > 120 || value.size() > 120 || context.size() > 120)
        return false;
    
    unsigned long int hash1 = hashFn(key); // get the hash corresponding to the key
    
    int bucketPtr = hash1 % ( (bucketCount * sizeof(Bucket)) + 4); // get a pointer corresponding to the hash
    
    // modify the pointer so its pointing to the beginning of a bucket
    if (bucketPtr < 4){
        bucketPtr = 4;
    }
    else if (bucketPtr % sizeof(Bucket) != 0){
        while (bucketPtr % sizeof(Bucket) != 0){
            bucketPtr--;
        }
    }
    
    DiskNode node(key, value, context); // create a new node with corresponding key, value, context
 
    openExisting(m_filename);
    
    //check if there are previously deleted nodes we can use
    /*deletedBucket dBucket;
    dBucket.ptr = 0;
    assert(m_file.read(dBucket, sizeof(bucketCount)));*/
    
    int ptrToNewNode;
    
    // this code is for reusing deleted nodes...it isn't functioning
    /*if (dBucket.ptr != 0 ){
        assert(m_file.write(node, dBucket.ptr));
        ptrToNewNode = dBucket.ptr;
    }
    else{
        assert(m_file.write(node, m_file.fileLength()));
        ptrToNewNode = m_file.fileLength() - sizeof(DiskNode);
    }*/
    
    assert(m_file.write(node, m_file.fileLength())); // insert the node into the on-disk file
    ptrToNewNode = m_file.fileLength() - sizeof(DiskNode);
    
    // read the pointer in the bucket
    int ptr_check = 0;
    Bucket bucket;
    m_file.read(bucket, bucketPtr);
    ptr_check = bucket.ptr;
    
    // if the bucket isn't pointing to anything the new node will be the first in the list
    if (ptr_check == 0){
        Bucket newBucket;
        newBucket.ptr = ptrToNewNode;
        assert(m_file.write(newBucket, bucketPtr));
    }
    else{ // if there is already a list, add the new node to the end of the list
        int new_ptr = 0;
        DiskNode node;
        m_file.read(node, ptr_check);
        new_ptr = node.next;
        
        while (new_ptr != 0){
            ptr_check = new_ptr;
            m_file.read(node, new_ptr);
            new_ptr = node.next;
        }
        
        node.next = ptrToNewNode;
        
        assert(m_file.write(node, ptr_check))
    }
    
    close();
    return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key){
    
    openExisting(m_filename);
    
    unsigned long int hash = hashFn(key); // get the hash corresponding to the key
    
    int location = hash % ( (bucketCount * sizeof(Bucket)) + 4); // get the pointer corresponding to the hash
    
    // modify the pointer so its pointing to the beginning of the bucket
    if (location < 4){
        location = 4;
    }
    else if (location % sizeof(Bucket) != 0){
        while (location % sizeof(Bucket) != 0){
            location--;
        }
    }
    
    // read the pointer in the bucket/hash table
    int read_ptr;
    assert(m_file.read(read_ptr, location));
    prevPtrWhenSearching = location;
    while (read_ptr != 0){ // while the end of the linked list isn't reached...
        
        
        DiskNode node;
        assert(m_file.read(node, read_ptr)); // check each node in the linked list
        
        if (strcmp(node.key, key.c_str()) == 0){ // if the key matches, return the iterator pointing to this node
            Iterator it(read_ptr, m_filename, key);
            close();
            return it;
        }
        
        prevPtrWhenSearching = read_ptr;
        read_ptr = node.next;

    }
    
    close();
    Iterator it; // if no node with matching key is found, return a iterator in an invalid state
    return it;
    
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context){
    // THIS FUNCTION IS NON FUNCTIONING
    
    assert(openExisting(m_filename));
    
    Iterator it = search(key);
    int numErased = 0;
    
    int read_ptr;
    assert(m_file.read(read_ptr, prevPtrWhenSearching));
    
    while (it.isValid()){
        if ((*it).key == key && (*it).value == value && (*it).context == context){
            int newDeletedNode = -1;
            if (prevPtrWhenSearching != 0 && prevPtrWhenSearching < (sizeof(deletedBucket) + sizeof (bucketCount) + (sizeof(Bucket) * bucketCount)))
            {
                int read_ptr;
                // PROBLEM: THIS READ FAILS
                assert(m_file.read(read_ptr, prevPtrWhenSearching));
                
                newDeletedNode = read_ptr;
    
            }
            else if ( prevPtrWhenSearching >= (sizeof(deletedBucket) + sizeof (bucketCount) + (sizeof(Bucket) * bucketCount)))
            {
                DiskNode node;
                m_file.read(node, prevPtrWhenSearching);
                
                newDeletedNode = node.next;
                
                node.next= 0;
                m_file.write(node, prevPtrWhenSearching);
            }
            
            if (newDeletedNode != -1){
                deletedBucket dBucket;
                m_file.read(dBucket, sizeof(bucketCount));
                
                if (dBucket.ptr != 0){
                    DiskNode node;
                    node.next = dBucket.ptr;
                    m_file.write(node, newDeletedNode);
                }
                
                dBucket.ptr = newDeletedNode;
                numErased++;
                m_file.write(dBucket, sizeof(bucketCount));
            }
            
        }
        ++it;
    }
    
    return numErased;
}

