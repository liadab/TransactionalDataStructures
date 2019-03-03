#pragma once

#include <memory>
#include "LNode.h"

#if defined(UNSAFE)

/**
 * this class is wrapping the Lnode so we would have easier control on its memory model.
 * this one is an unsafe memory wrapper
 */
template <typename key_t, typename val_t>
class LNodeWrapper {
public:
    LNodeWrapper() : m_node(NULL), deleted(std::make_shared<bool>(false)) {}

    explicit LNodeWrapper(key_t key) {
        m_node = new LNode<key_t, val_t>(key);
        deleted = std::make_shared<bool>(false);
    }

    LNodeWrapper(key_t key, val_t val) : LNodeWrapper(std::move(key)){
        m_node->m_val = std::move(val);
        deleted = std::make_shared<bool>(false);
    }

    bool operator==(const LNodeWrapper<key_t, val_t>& other) const {
        return m_node == other.m_node;
    }

    bool operator!=(const LNodeWrapper<key_t, val_t>& other) const{
        return m_node != other.m_node;
    }

    bool is_null() {
        return m_node == NULL;
    }

    bool is_not_null() {
        return m_node != NULL;
    }

    bool is_deleted() const {
        return *deleted;
    }

    void delete_wrapped_node() {
        *deleted = false;
    }

    LNode<key_t, val_t>* operator->() {
        return m_node;
    }


    const LNode<key_t, val_t>* operator->() const {
        return m_node;
    }

    size_t hash() const {
        auto hasher = std::hash<LNode<key_t, val_t>*>();
        return hasher(m_node);
    }

    friend std::ostream& operator<< (std::ostream& stream, const LNodeWrapper<key_t, val_t>& node) {
        if(!node.m_node) {
            stream << "null";
        } else {
            stream << "[" << node->m_key << ": ";
            if (node->m_val) {
                stream << node->m_val;
            } else {
                stream << "None";
            }
            stream << "] ";
        }
        return stream;
    }

private:
    LNode<key_t, val_t>* m_node;
    std::shared_ptr<bool> deleted;
};

namespace std {

    template <typename key_t, typename val_t>
    struct hash<LNodeWrapper<key_t, val_t>>
    {
        std::size_t operator()(const LNodeWrapper<key_t, val_t>& k) const
        {
            return k.hash();
        }
    };

}

#elif  defined(DEBRA)

/**
 * this class is wrapping the Lnode so we would have easier control on its memory model.
 * this one is an unsafe memory wrapper
 */
template <typename key_t, typename val_t>
class LNodeWrapper {
public:
    LNodeWrapper() : m_node(NULL) , deleted(std::make_shared<bool>(false)) {}

    LNodeWrapper(LNode<key_t, val_t>* node) : m_node(node) , deleted(std::make_shared<bool>(false)) {}

    LNodeWrapper(const LNodeWrapper<key_t, val_t>& other) : m_node(other.m_node) , deleted(std::make_shared<bool>(false)) {}

    bool operator==(const LNodeWrapper<key_t, val_t>& other) const {
        return m_node == other.m_node;
    }

    bool operator!=(const LNodeWrapper<key_t, val_t>& other) const{
        return m_node != other.m_node;
    }

    bool is_null() {
        return m_node == NULL;
    }

    bool is_not_null() {
        return m_node != NULL;
    }

    bool is_deleted() const {
        return *deleted;
    }

    //return the wrapped node and this wrapper is considered deleted
    LNode<key_t, val_t>* delete_wrapped_node() {
//        *deleted = true;
        return m_node;
    }

    LNode<key_t, val_t>* operator->() {
        return m_node;
    }


    const LNode<key_t, val_t>* operator->() const {
        return m_node;
    }

    size_t hash() const {
        auto hasher = std::hash<LNode<key_t, val_t>*>();
        return hasher(m_node);
    }

    friend std::ostream& operator<< (std::ostream& stream, const LNodeWrapper<key_t, val_t>& node) {
        if(!node.m_node) {
            stream << "null";
        } else {
            stream << "[" << node->m_key << ": ";
            if (node->m_val) {
                stream << node->m_val;
            } else {
                stream << "None";
            }
            stream << "] ";
        }
        return stream;
    }

private:
    LNode<key_t, val_t>* m_node;
    std::shared_ptr<bool> deleted;
};

namespace std {

    template <typename key_t, typename val_t>
    struct hash<LNodeWrapper<key_t, val_t>>
    {
        std::size_t operator()(const LNodeWrapper<key_t, val_t>& k) const
        {
            return k.hash();
        }
    };

}

#else


/**
 * this class is wrapping the Lnode so we would have easier control on its memory model.
 * for now its just a shared_ptr wrapper
 */
template <typename key_t, typename val_t>
class LNodeWrapper {
public:
    LNodeWrapper() {
        deleted = std::make_shared<bool>(false);
    };

    explicit LNodeWrapper(key_t key) {
        m_node = std::make_shared<LNode<key_t, val_t>>(std::move(key));
        deleted = std::make_shared<bool>(false);
    }

    LNodeWrapper(key_t key, val_t val) : LNodeWrapper(std::move(key)){
        m_node->m_val = std::move(val);
    }

    bool operator==(const LNodeWrapper<key_t, val_t>& other) const {
        return m_node == other.m_node;
    }

    bool operator!=(const LNodeWrapper<key_t, val_t>& other) const{
        return m_node != other.m_node;
    }

    bool is_null() {
        return !static_cast<bool>(m_node);
    }

    bool is_not_null() {
        return static_cast<bool>(m_node);
    }

    bool is_deleted() const {
        return *deleted;
    }

    void delete_wrapped_node() {
        *deleted = false;
    }

    LNode<key_t, val_t>* operator->() {
        return m_node.get();
    }


    const LNode<key_t, val_t>* operator->() const {
        return m_node.get();
    }

    size_t hash() const {
        auto hasher = std::hash<LNode<key_t, val_t>*>();
        return hasher(m_node.get());
    }

    friend std::ostream& operator<< (std::ostream& stream, const LNodeWrapper<key_t, val_t>& node) {
        if(!node.m_node) {
            stream << "null";
        } else {
            stream << "[" << node->m_key << ": ";
            if (node->m_val) {
                stream << node->m_val;
            } else {
                stream << "None";
            }
            stream << "] ";
        }
        return stream;
    }

private:
    std::shared_ptr<LNode<key_t, val_t>> m_node;
    std::shared_ptr<bool> deleted;
};

namespace std {

    template <typename key_t, typename val_t>
    struct hash<LNodeWrapper<key_t, val_t>>
    {
        std::size_t operator()(const LNodeWrapper<key_t, val_t>& k) const
        {
            return k.hash();
        }
    };

}

#endif