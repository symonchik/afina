#include "SimpleLRU.h"

namespace Afina {
namespace Backend {


void SimpleLRU::delete_last()
{
    lru_node *del_node = _lru_tail;
    // counting a new size
    std::size_t delta_sz = del_node->key.size() + del_node->value.size(); 
    _lru_index.erase(del_node->key);  
    if (_lru_head.get() == _lru_tail) 
    {
        // no more nodes
        _lru_head.reset(nullptr);
    } else {
        _lru_tail = _lru_tail->prev; // new tail
        _lru_tail->next.reset(nullptr); // transfer pointer
    }
    _cur_size -= delta_sz;
}

void SimpleLRU::insert_node(lru_node *node)
{
    if (_lru_head.get()) {
        // something in head
        // with save head
        _lru_head.get()->prev = node;
    } else 
    {
        _lru_tail = node;
    }
    node->next = std::move(_lru_head);
    // Destroys the object currently managed by the unique_ptr (if any) and takes ownership of p.
    // If p is a null pointer (such as a default-initialized pointer), the unique_ptr becomes empty, managing no object after the call.
    // To release the ownership of the stored pointer without destroying it, use member function release instead.
    _lru_head.reset(node);            
}

void SimpleLRU::node_to_head(lru_node *node)
{
    if (_lru_head.get() == node)
    {
        return;
    }
    if (!node->next.get()) 
    {
        _lru_tail = node->prev;
        _lru_head.get()->prev = node;
        node->next = std::move(_lru_head);
        _lru_head = std::move(node->prev->next);

    } else {
        auto tmp = std::move(_lru_head);
        _lru_head = std::move(node->prev->next);
        node->prev->next = std::move(node->next);
        tmp.get()->prev = node;
        node->next = std::move(tmp);
        node->prev->next->prev = node->prev;
    }
}

void SimpleLRU::PutImpl(const std::string &key, const std::string &value, const std::size_t entry_size)
{
    // free memory for
    while (_cur_size + entry_size > _max_size) {
        delete_last();
    }
    lru_node *new_node = new lru_node{key, value, nullptr, nullptr};
    insert_node(new_node);
    // refresh map
    // Inserts element(s) into the container, if the container doesn't already contain an element with an equivalent key.
    _lru_index.insert({std::reference_wrapper<const std::string>(new_node->key),
                       std::reference_wrapper<lru_node>(*new_node)}); 
    _cur_size += entry_size;                                         
    return;
}

void SimpleLRU::SetImpl(lru_node &node, const std::string &value)
{
    // new value for node
    int delta_sz = value.size() - node.value.size();
    node_to_head(&node);                           
    while (_cur_size + delta_sz > _max_size)        
    {
        delete_last();
    }
    node.value = value;
    _cur_size += delta_sz; 
    return;
}



// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    std::size_t entry_size = key.size() + value.size(); 
    if (entry_size > _max_size)                        
    {
        return false;
    }
    auto node = _lru_index.find(key); 
    if (node != _lru_index.end())     
    {
        SetImpl((node->second).get(), value);
    } else 
    {
        PutImpl(key, value, entry_size);
    }
    return true; 
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    std::size_t entry_size = key.size() + value.size(); 
    if (entry_size > _max_size)                      
    {
        return false;
    }
    if (_lru_index.find(key) == _lru_index.end()) {
        PutImpl(key, value, entry_size);
        return true;
    } else {
        return false;
    }
}
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    std::size_t entry_size = key.size() + value.size(); 
    if (entry_size > _max_size)    
    {
        return false;
    }
    auto node = _lru_index.find(key);
    if (node != _lru_index.end()) {
        SetImpl((node->second).get(), value);
    } else {
        return false;
    }
}
// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    auto tmp = _lru_index.find(key);
    if (tmp == _lru_index.end()) 
    {
        return false;
    }
    lru_node *node = &(tmp->second.get());
    _lru_index.erase(tmp); 
    std::size_t node_size = node->key.size() + node->value.size();
    if (node == _lru_head.get())
    {
        if (!node->next.get()) 
        {
            _lru_head.reset();
            _lru_tail = nullptr;
        } else {
            node->next.get()->prev = nullptr;
            _lru_head = std::move(node->next);
        }
    } else if (!node->next.get()) 
    {
        _lru_tail = node->prev;
        node->prev->next.reset();
    } else {
        node->next.get()->prev = node->prev;
        node->prev->next = std::move(node->next);
    }
    _cur_size -= node_size; 
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    auto node = _lru_index.find(key);
    if (node == _lru_index.end()) {
        return false;
    }
    value = node->second.get().value;
    node_to_head(&node->second.get());
    return true;
}

} // namespace Backend
} // namespace Afina
