//
// Created by 唐仁初 on 2022/9/14.
//

#include "GossipSlot.h"


namespace gossip::server{

    SlotVersion GossipSlot::insertOrUpdate(const std::string &key, const std::string &value) {
        std::lock_guard<std::mutex> lg(mtx);
        map_[key] = value;
        version_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        mem_use_ += key.size() + value.size();
        return version_;
    }

    SlotVersion GossipSlot::insertOrUpdate(const std::string &key, const std::string &value, SlotVersion version) {
        std::lock_guard<std::mutex> lg(mtx);
        if (version < version_)
            return -1;
        map_[key] = value;
        version_ = version;
        mem_use_ += key.size() + value.size();
        return version_;
    }

    SlotVersion GossipSlot::remove(const std::string &key) {
        std::lock_guard<std::mutex> lg(mtx);

        auto it = map_.find(key);
        if(it == map_.end())
            return -1;

        mem_use_ -= (it->first.size() + it->second.size());
        map_.erase(it);
        version_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        return version_;
    }

    SlotVersion GossipSlot::remove(const std::string &key, SlotVersion version) {
        std::lock_guard<std::mutex> lg(mtx);

        if (version < version_)
            return -1;

        auto it = map_.find(key);
        if(it == map_.end())
            return -1;

        mem_use_ -= (it->first.size() + it->second.size());
        map_.erase(it);
        version_ = version;
        return version_;
    }

    SlotVersion GossipSlot::compareAndMergeSlot(SlotValues values, SlotVersion version) {
        std::lock_guard<std::mutex> lg(mtx);
        if (version < version_)
            return -1;
        map_ = std::move(values);
        return version_;
    }

    std::pair<std::string, SlotVersion> GossipSlot::find(const std::string &key) {
        std::lock_guard<std::mutex> lg(mtx);

        auto it = map_.find(key);
        if (it == map_.end())
            return {"", 0};
        return {it->second, version_};
    }


}


