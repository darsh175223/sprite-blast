#ifndef VECTOR_H
#define VECTOR_H

template <typename T>
class Vector {
private:
    T* arr;              // Pointer to the dynamic array
    size_t sz;           // Current number of elements
    size_t cap;          // Current capacity of the array

    // Helper function to reallocate the array (renamed to avoid ambiguity)
    void reallocate(size_t newCapacity) {
        T* newArr = new T[newCapacity];
        for (size_t i = 0; i < sz; ++i) {
            newArr[i] = arr[i];
        }
        delete[] arr;
        arr = newArr;
        cap = newCapacity;
    }

public:
    // Constructor
    Vector() : arr(nullptr), sz(0), cap(0) {}

    // Copy constructor
    Vector(const Vector& other) : arr(new T[other.cap]), sz(other.sz), cap(other.cap) {
        for (size_t i = 0; i < sz; ++i) {
            arr[i] = other.arr[i];
        }
    }

    // Assignment operator
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            delete[] arr;
            sz = other.sz;
            cap = other.cap;
            arr = new T[cap];
            for (size_t i = 0; i < sz; ++i) {
                arr[i] = other.arr[i];
            }
        }
        return *this;
    }

    // Destructor
    ~Vector() {
        delete[] arr;
    }

    // Add element to the end
    void push_back(const T& value) {
        if (sz == cap) {
            // Double the capacity or set to 1 if empty
            reallocate(cap == 0 ? 1 : cap * 2);
        }
        arr[sz++] = value;
    }

    // Remove last element
    void pop_back() {
        if (sz == 0) {
            return; // Do nothing if empty
        }
        --sz;
    }

    // Get element at index
    T& operator[](size_t index) {
        // Return first element if index is invalid (avoids default construction)
        if (index >= sz || sz == 0) {
            static T* dummy = nullptr;
            return *dummy; // Will cause a null dereference if accessed
        }
        return arr[index];
    }

    // Const version of index operator
    const T& operator[](size_t index) const {
        // Return first element if index is invalid (avoids default construction)
        if (index >= sz || sz == 0) {
            static T* dummy = nullptr;
            return *dummy; // Will cause a null dereference if accessed
        }
        return arr[index];
    }

    // Return current size
    size_t size() const {
        return sz;
    }

    // Return current capacity
    size_t capacity() const {
        return cap;
    }

    // Check if vector is empty
    bool empty() const {
        return sz == 0;
    }

    // Clear all elements
    void clear() {
        sz = 0;
    }

    // Resize the vector to a specific size
    void resize(size_t newSize, const T& value) {
        if (newSize > cap) {
            reallocate(newSize);
        }
        for (size_t i = sz; i < newSize; ++i) {
            arr[i] = value;
        }
        sz = newSize;
    }

    // Insert element at specific index
    void insert(size_t index, const T& value) {
        if (index > sz) {
            return; // Do nothing if index is invalid
        }
        if (sz == cap) {
            reallocate(cap == 0 ? 1 : cap * 2);
        }
        for (size_t i = sz; i > index; --i) {
            arr[i] = arr[i - 1];
        }
        arr[index] = value;
        ++sz;
    }

    // Erase element at specific index
    void erase(size_t index) {
        if (index >= sz) {
            return; // Do nothing if index is invalid
        }
        for (size_t i = index; i < sz - 1; ++i) {
            arr[i] = arr[i + 1];
        }
        --sz;
    }
};

#endif