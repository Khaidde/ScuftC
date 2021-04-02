#pragma once

#include <stdint.h>

#include <stdexcept>
#include <string>

#ifndef NDEBUG
#define ASSERT(assertion, errMsg)                                                                              \
    do {                                                                                                       \
        if (!(assertion))                                                                                      \
            throw std::runtime_error("--ASSERTION ERROR-- in " + std::string(__FILE__) + "(" +                 \
                                     std::to_string(__LINE__) + "): \n\t" + __PRETTY_FUNCTION__ + "\n\t    " + \
                                     std::to_string(__LINE__) + "    " errMsg);                                \
    } while (false)
#else
#define ASSERT(assertion, errMsg) \
    do {                          \
    } while (false)
#endif

template <class T>
struct SList {
    SList() = default;
    ~SList() = default;
    void destroy() {
        delete[] data;
        printf("List of size=%zu freed!\n", size);
    }
    void resize(size_t newCapacity) {
        ASSERT(newCapacity > 0, "Capacity of list must be greater than 0, newCapacity=" + std::to_string(newCapacity));
        capacity = newCapacity;
        data = reinterpret_cast<T*>(realloc(data, capacity * sizeof(*data)));
        if (!data) throw std::bad_alloc();
    }
    T& operator[](size_t index) {
        ASSERT(index < size, "Index=" + std::to_string(index) + " out of bounds(size=" + std::to_string(size) + ")");
        return data[index];
    }
    const T& operator[](size_t index) const {
        ASSERT(index < size, "Index=" + std::to_string(index) + " out of bounds(size=" + std::to_string(size) + ")");
        return data[index];
    }
    T& begin() {
        ASSERT(size > 0, "Can't access begin of empty list");
        return data[0];
    }
    T& end() {
        ASSERT(size > 0, "Can't access end of empty list");
        return data[size - 1];
    }
    void push(const T& item) {
        if (size == capacity) {
            resize(size + (size >> 1) + 1);
        }
        data[size++] = std::move(item);
    }
    T pop() { return data[size--]; }
    bool empty() { return size == 0; }
    size_t size;
    size_t capacity;

   private:
    T* data;
};

template <class Key, class T, int (*hash)(Key), bool (*equal)(Key, Key)>
struct SHashTable {
    void init(size_t initBucketSize) {
        static_assert(std::is_pointer<T>::value, "Non-pointer value hashtable must have a pointer value type");
        init_empty_val(initBucketSize, nullptr);
    }
    void init_empty_val(size_t initBucketSize, T emptyVal) {
        ASSERT(initBucketSize > 0,
               "Bucket size of hashtable must be greater than 0, initBucketSize=" + std::to_string(initBucketSize));
        table = {};
        table.resize(initBucketSize);

        mapCapacity = initBucketSize;
        mapping = new int[initBucketSize];
        memset(mapping, EMPTY_INDEX, initBucketSize * sizeof(*mapping));
    }
    void destroy() {
        table.destroy();
        delete[] mapping;
        printf("Hashtable of capacity=%zu freed!\n", mapCapacity);
    }
    T find(Key key) {
        int kHash = mod_hash(key);
        for (int i = 0; i < mapCapacity; i++) {
            int tableIndex = mapping[(kHash + i) % mapCapacity];
            if (tableIndex == EMPTY_INDEX) {
                break;
            } else if (table[tableIndex].keyHash == kHash && equal(table[tableIndex].key, key)) {
                return table[tableIndex].value;
            }
        }
        return emptyVal;
    }
    void set(Key key, T val) {
        ASSERT(find(key) == emptyVal, "Duplicate key in hashtable");
        fast_set(key, val);
    }
    void fast_set(Key key, T val) {
        if (mapCapacity >> 1 <= table.size) {
            delete[] mapping;
            mapCapacity = (mapCapacity << 1) + 1;

            mapping = reinterpret_cast<T*>(malloc(mapCapacity * sizeof(*mapping)));
            if (!mapping) throw std::bad_alloc();
            memset(mapping, EMPTY_INDEX, mapCapacity * sizeof(*mapping));

            for (int i = 0; i < table.size; i++) {
                internal_allocate_entry(table[i].key, i);
            }
        }

        Entry entry{};
        entry.key = key;
        entry.value = val;
        table.push(std::move(entry));
        internal_allocate_entry(key, table.size - 1);
    }

    //    private:
    void internal_allocate_entry(Key key, int index) {
        Entry& entry = table[index];
        entry.keyHash = mod_hash(entry.key);
        for (int i = 0; i < mapCapacity; i++) {
            int off = (entry.keyHash + i) % mapCapacity;
            if (mapping[off] == EMPTY_INDEX) {
                mapping[off] = index;
                break;
            }
        }
    }
    int mod_hash(Key k) { return (hash(k) & 0x7fffffff) % mapCapacity; }

    size_t capacity;

    struct Entry {
        int keyHash;
        Key key;
        T value;
    };
    SList<Entry> table;
    T emptyVal;

    static constexpr char EMPTY_INDEX = -1;
    int* mapping;  // map from hashes to table indices
    size_t mapCapacity;
};

static int str_ptr_hash(std::string* str) {
    int hash = 0;
    for (int i = 0; i < str->length(); i++) {
        hash = (hash << 5) - hash + str->at(i);
    }
    return hash;
}

static bool str_ptr_equals(std::string* a, std::string* b) {
    if (a->length() != b->length()) {
        return false;
    }
    for (int i = 0; i < a->length(); i++) {
        if (a->at(i) != b->at(i)) return false;
    }
    return true;
}

static int hash_table_test() {
    try {
        SHashTable<std::string*, int, str_ptr_hash, str_ptr_equals> table = {};
        table.init_empty_val(16, 0);

        static constexpr size_t NUM_ITER = 8;
        std::string str[NUM_ITER];
        for (int i = 0; i < NUM_ITER; i++) {
            str[i] = std::to_string(i * 21) + std::string((i * 3) % 6, '-') + "test";
            table.set(&str[i], i * 13 + 2);
        }
        for (int i = NUM_ITER - 1; i >= 0; i--) {
            std::string f = std::to_string(i * 21) + std::string((i * 3) % 6, '-') + "test";
            printf("find %s(%d) -> %d\n", f.c_str(), table.mod_hash(&f), table.find(&f));
        }

        std::string f = "other";
        printf("tried to find '%s' and found: %d\n", f.c_str(), table.find(&f));

        f = "blank";
        printf("tried to find '%s' and found: %d\n", f.c_str(), table.find(&f));

        for (int i = 0; i < table.mapCapacity; i++) {
            printf("::%d:%d\n", i, table.mapping[i]);
        }

        table.destroy();
    } catch (std::exception& e) {
        printf("%s", e.what());
    }
    return 0;
}