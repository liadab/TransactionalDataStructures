//template <typename key_t, typename val_t>
//bool LNode<key_t,val_t>::tryLock() {
//    uint64_t l = m_version_mask;
//    if ((l & LOCK_MASK) != 0) {
//        return false;
//    }
//    uint64_t locked = l | LOCK_MASK;
//    return m_version_mask.compare_exchange_strong(l, locked);
//}