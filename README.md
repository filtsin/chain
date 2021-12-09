Just simple example of using chain for cpp containers.

Just include **chain.hpp**

Chain the same containers:
```cpp
std::vector<int> a1{0, 1, 2};
std::vector<int> a2{3, 4};
std::vector<int> a3;

for (auto &v : make_chain(a1, a2, a3)) {
    // 0 1 2 3 4
    std::cout << v << " ";
}
```

Chain different containers:
```cpp
std::list<int> a1{0, 1};
std::vector<int> a2{2};
std::array<int, 2> a3{3, 4};

for (auto &v : make_chain(a1, a2, a3)) {
    std::cout << v << " ";
}
```

Chain different types:

```cpp
struct Foo {
    int a;
    operator int() {
        return a;
    }
};

int main() {
    std::vector<int> a1{0, 1};
    std::vector<long> a2{2};
    std::vector<Foo> a3{Foo{3}};

    for (int v : make_chain(a1, a2, a3)) {
        // 0 1 2 3
        std::cout << v << " ";
    }
}
```

Not working with not convertible types:
```cpp
std::vector<int> a1{0, 1};

std::vector<std::string> a2{"hello"};

for (int v : make_chain(a1, a2)) {
    // Fail to compile with message: Value type of iterators bust be the same
    std::cout << v << " ";
}
```
