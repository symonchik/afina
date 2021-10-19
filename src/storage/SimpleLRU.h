#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <iostream>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in
    // the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node *_lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    // std::map is a sorted associative container that contains key-value pairs with unique keys.
    // std::reference_wrapper is a class template that wraps a reference in a copyable, assignable object.
    // std::less function object for performing comparisons. Unless specialized, invokes operator< on type T.
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;

    // Delete last node
    void delete_last();
    // Insert new node 
    void insert_node(lru_node *node);
    // Insert new node to head
    void node_to_head(lru_node *node);
    // Put new node implementation 
    void PutImpl(const std::string &key, const std::string &value, const std::size_t entry_size);
    // Set new node implementation 
    void SetImpl(lru_node &node, const std::string &value);

public:
    SimpleLRU(size_t max_size = 1024)
        : _max_size(max_size), _cur_size(0), _lru_tail(nullptr), _lru_index(), _lru_head() {}

    ~SimpleLRU() {
        _lru_index.clear();
        while (_lru_tail && _lru_tail != _lru_head.get()) {
            _lru_tail = _lru_tail->prev;
            _lru_tail->next.reset();
        }
        _lru_head.reset();
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;  
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
