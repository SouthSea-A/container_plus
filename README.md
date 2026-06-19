# container_plus
## 概述
container_plus是一个支持快速访问、追加、插入、删除的线性容器，按插入顺序维护数据次序。同时，支持正反随机迭代器、STL算法、自定义内存分配器。需C++23及以上支持
## 特性简介
- 无额外依赖，仅标准库
- 快速顺序遍历访问、随机访问
- 快速追加
- 快速顺序或步进插入、随机插入
- 快速删除
- 元素大量连续储存，内存碎片率低
- 支持正反随机访问迭代器
- 支持STL算法
- 支持自定义内存分配器

## 快速开始
### 基本用法
```c++
#include "manager.h"

// 定义容器类型（默认块大小 32768）
using MyContainer = container_plus<MyType>;

// 创建容器
MyContainer container;

// 添加元素
container.push_back(MyType("hello"));
container.emplace_back("world", 42);

// 访问元素
auto& ref = container[0];          // 随机访问
auto& front = container.front();   // 访问第一个
auto& back = container.back();     // 访问最后一个

// 获取大小
std::size_t size = container.size();
bool empty = container.empty();

// 删除元素
container.pop_back();              // 删除最后一个
container.erase(container.begin()); // 删除第一个
```

## 类型定义
### 容器类型别名
```c++
template<typename T, typename Allocator = std::allocator<T>>
using container_plus = manager<T, BLOCK_SIZE_DEFAULT, Allocator>;
```
- `T`：存储的元素类型
- `Allocator`：分配器（可选，默认 `std::allocator<T>`）
- *如需要自定义单块大小，请用`manager<T, MAX, Allocator>`，修改`MAX`，自定义别名，块的大小MAX一定程度上决定整个容器的性能好坏和内存占用情况，这里默认为32768*

## 迭代器
容器提供完整的随机访问迭代器支持。
## 迭代器类型
|类型|说明|
|-|-|
|`iterator`|可读写迭代器|
|`const_iterator`|只读迭代器|
|`reverse_iterator`|反向迭代器|
|`const_reverse_iterator`|常量反向迭代器|
## 获取迭代器
```cpp
container_plus<T> container;

// 正向迭代器
auto it_begin = container.begin();
auto it_end = container.end();
auto cit_begin = container.cbegin();
auto cit_end = container.cend();

// 反向迭代器
auto rit_begin = container.rbegin();
auto rit_end = container.rend();
auto crit_begin = container.crbegin();
auto crit_end = container.crend();
```
## 迭代器操作
```cpp
// 遍历
for (auto it = container.begin(); it != container.end(); ++it) {
    // 处理 *it
}

// 范围 for 循环（推荐）
for (const auto& elem : container) {
    // 处理 elem
}

// 反向遍历
for (auto it = container.rbegin(); it != container.rend(); ++it) {
    // 处理 *it
}

// 随机访问
auto it = container.begin() + 5;
auto val = it[3];

// 迭代器差值
auto dist = container.end() - container.begin();

// 比较操作
if (it1 < it2) { /* ... */ }
if (it1 == container.end()) { /* ... */ }
```
## 元素访问
### operator[]
```cpp
// 读写访问
container_plus<T> container;
T& ref = container[10];
const T& cref = const_container[10];

// 不进行边界检查，索引越界导致未定义行为
```
### at()
```cpp
// 带边界检查的访问
try {
    T& ref = container.at(10);
} catch (const std::runtime_error& e) {
    // 索引无效
}
```
### front() / back()
```cpp
// 访问首元素
T& front = container.front();
const T& cfront = const_container.front();

// 访问尾元素
T& back = container.back();
const T& cback = const_container.back();

// 容器为空时抛出 std::runtime_error
```
## 容量操作
```cpp
// 获取元素数量
std::size_t size = container.size();

// 判断是否为空
bool empty = container.empty();

// 手动压缩内存（整理碎片）
container.shrink_to_fit();

// 清空所有元素
container.clear();
```
## 添加元素
### push_back
```cpp
// 复制添加
T obj;
container.push_back(obj);

// 移动添加
container.push_back(std::move(obj));
```
### emplace_back
```cpp
// 原地构造，避免拷贝/移动
container.emplace_back("param1", 42, 3.14);
container.emplace_back();  // 默认构造
```
### insert
```cpp
// 在指定位置插入（复制）
auto it = container.begin() + 5;
container.insert(it, T("new"));

// 在指定位置插入（移动）
container.insert(it, std::move(T("new")));

// 使用反向迭代器插入
auto rit = container.rbegin();
container.insert(rit, T("new"));

// 返回指向新插入元素的迭代器
auto new_it = container.insert(container.begin(), T("first"));
```
### emplace（原地构造插入）
```cpp
// 在指定位置原地构造
auto it = container.begin() + 3;
container.emplace(it, "param1", 42);

// 返回新元素的引用
auto& ref = container.emplace_back("param1", 42);
```
## 删除元素
### erase
```cpp
// 删除单个元素
auto it = container.begin() + 3;
container.erase(it);

// 删除迭代器指向的元素（常量迭代器也可用）
const auto cit = container.cbegin() + 5;
container.erase(cit);

// 删除反向迭代器指向的元素
auto rit = container.rbegin();
container.erase(rit);
```
### pop_back
```cpp
// 删除最后一个元素
container.pop_back();

// 容器为空时行为未定义
```
### clear
```cpp
// 清空所有元素
container.clear();
```
## 完整示例
```cpp
#include <iostream>
#include <string>
#include "manager.h"

using StringContainer = container_plus<std::string>;

int main() {
    // 1. 创建容器
    StringContainer container;
    
    // 2. 添加元素
    container.emplace_back("first");
    container.emplace_back("second");
    container.emplace_back("third");
    container.push_back("fourth");
    
    std::cout << "Size: " << container.size() << std::endl;  // 4
    
    // 3. 遍历
    for (const auto& str : container) {
        std::cout << str << " ";
    }
    std::cout << std::endl;  // first second third fourth
    
    // 4. 插入元素
    auto it = container.begin() + 2;
    container.insert(it, "inserted");
    // 现在: first second inserted third fourth
    
    // 5. 访问元素
    std::cout << "First: " << container.front() << std::endl;
    std::cout << "Last: " << container.back() << std::endl;
    std::cout << "Index 2: " << container.at(2) << std::endl;
    
    // 6. 删除元素
    container.pop_back();  // 删除 fourth
    container.erase(container.begin());  // 删除 first
    // 现在: second inserted third
    
    // 7. 修改元素
    container[1] = "modified";
    
    // 8. 清空
    container.clear();
    std::cout << "Empty: " << (container.empty() ? "yes" : "no") << std::endl;
    
    return 0;
}
```
## 构造与析构
```cpp
// 默认构造
container_plus<T> container;

// 使用自定义分配器构造
MyAllocator alloc;
container_plus<T, MyAllocator> container(alloc);
```

## 自定义分配器
```cpp
// 使用自定义分配器
template<typename T>
struct MyAllocator {
    // 标准分配器接口...
};

using MyContainer = container_plus<int, MyAllocator<int>>;
MyContainer container(MyAllocator<int>());
```
## STL 算法示例
### 1. 排序算法
```cpp
#include <algorithm>
#include <string>
#include <vector>

using Container = container_plus<int>;

Container container;
container.emplace_back(5);
container.emplace_back(2);
container.emplace_back(8);
container.emplace_back(1);
container.emplace_back(9);

// 排序（默认升序）
std::sort(container.begin(), container.end());
// 结果: 1, 2, 5, 8, 9

// 降序排序
std::sort(container.begin(), container.end(), std::greater<int>());
// 结果: 9, 8, 5, 2, 1

// 部分排序（前3个元素排序）
std::partial_sort(container.begin(), container.begin() + 3, container.end());
// 结果: 1, 2, 5, 9, 8 (前3个有序)

// 稳定排序（保持相等元素的相对顺序）
struct Person {
    std::string name;
    int age;
    bool operator<(const Person& other) const { return age < other.age; }
};

container_plus<Person> people;
people.emplace_back("Alice", 30);
people.emplace_back("Bob", 25);
people.emplace_back("Charlie", 30);

std::stable_sort(people.begin(), people.end());
```
### 2. 查找算法
```cpp
#include <algorithm>
#include <string>

Container container;
for (int i = 0; i < 100; ++i) {
    container.emplace_back(i);
}

// 查找指定值
auto it = std::find(container.begin(), container.end(), 42);
if (it != container.end()) {
    std::cout << "Found: " << *it << std::endl;
}

// 查找满足条件的元素（第一个偶数）
auto it_even = std::find_if(container.begin(), container.end(), 
    [](int x) { return x % 2 == 0; });

// 查找不满足条件的元素（第一个奇数）
auto it_odd = std::find_if_not(container.begin(), container.end(),
    [](int x) { return x % 2 == 0; });

// 二分查找（需要先排序）
std::sort(container.begin(), container.end());
bool exists = std::binary_search(container.begin(), container.end(), 42);

// 查找子区间（搜索连续子序列）
std::vector<int> pattern = {42, 43, 44};
auto it_pattern = std::search(container.begin(), container.end(),
    pattern.begin(), pattern.end());

// 查找最后一个满足条件的元素
auto it_last = std::find_if(container.rbegin(), container.rend(),
    [](int x) { return x > 50; });
```

## 时间复杂度

|操作类型|操作|时间复杂度|说明|
|-|-|-|-|
|访问||||
||`operator[]`|$$O(log B)$$|$$B = 块数 ≈ N / MAX$$|
||`at()`|$$O(log B)$$|同 `operator[]`|
||`front()`|$$O(1)$$|直接访问首元素|
||`back()`|$$O(1)$$|直接访问尾元素|
|迭代器操作||||
||`begin() / end()`|$$O(1)$$||
||`cbegin() / cend()`|$$O(1)$$||
||`iterator++ / --`|$$O(1)$$||
||`++ / --iterator`|$$O(1)$$||
||`iterator += n`|$$O(log B)$$|需要重新定位|
||`iterator -= n`|$$O(log B)$$|需要重新定位|
||`iterator - iterator`|$$O(1)$$||
|插入||||
||`push_back`|$$摊还 O(1)$$|尾部追加|
||`emplace_back`|$$摊还 O(1)$$|尾部原地构造|
||`insert (尾部)`|$$摊还 O(1)$$||
||`insert (中间)`|$$O(log B + M)$$|$$M = MAX/2$$|
||`emplace (中间)`|$$O(log B + M)$$|同 `insert`|
|删除||||
||`pop_back`|$$O(1)$$||
||`erase (尾部)`|$$O(1)$$||
||`erase (中间)`|$$O(log B + M)$$||
|容量操作||||
||`size()`|$$O(1)$$||
||`empty()`|$$O(1)$$||
||`clear()`|$$O(N)$$|$$N = 元素总数$$|
||`shrink_to_fit()`|$$O(N)$$|遍历所有页并压缩|

## 性能测试
### 测试硬件：U9-275HX、DDR5-5600MT/s
### 测试系统：Windows11-25H2-26200.8655

### 元素追加测试
- 128字节每元素追加1200万次，重复测试50次，统一调用`emplace_back`，之后还有64字节、32字节、16字节的1200万次追加，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时(ms)|
|-|-|
|container_plus|1618.42|
|std::vector|1971.41|
|std::deque|624.65|
|std::list|615.97|

### 元素步进2插入测试
- 每次插入从`begin`开始重新计算插入位置
- 在原有的1200万128字节每元素的数据上插入600万次，重复测试50次，统一调用`insert`，之后还有64字节、32字节、16字节的600万次步进2插入，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时(ms)|
|-|-|
|container_plus|1618.42|
|std::vector|>3600000|
|std::deque|>3600000|
|std::list|>3600000|

### 元素随机插入测试
- 每次插入由随机生成的位置索引重新计算插入迭代器，插入位置范围为`[0,N)`，`N`为当前容器元素个数，`N`随插入而增长
- 在原有的1200万128字节每元素的数据上随机插入600万次，重复测试50次，统一调用`insert`，之后还有64字节、32字节、16字节的600万次随机插入，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时，含随机位置生成时间(ms)|
|-|-|
|container_plus|11208.72|
|std::vector|>3600000|
|std::deque|>3600000|
|std::list|>3600000|

### 元素遍历访问测试
- 每次访问由迭代器的左自增和解引用完成
- 在原有的1200万128字节每元素的数据上遍历访问，重复测试50次，统一使用`iterator`访问，之后还有1200万的64字节、32字节、16字节数据量的遍历访问，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时(ms)|
|-|-|
|container_plus|185.44|
|std::vector|7.56|
|std::deque|10.17|
|std::list|785.36|

### 元素随机访问测试
- 每次访问由随机生成的位置索引重新计算迭代器及其解引用完成
- 在原有的1200万128字节每元素的数据上随机访问1200万次，重复测试50次，统一使用`iterator`访问，之后还有1200万的64字节、32字节、16字节数据量的1200万次随机访问，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时，含随机位置生成时间(ms)|
|-|-|
|container_plus|2141.36|
|std::vector|284.25|
|std::deque|657.38|
|std::list|>3600000|

### 元素随机删除测试
- 每次删除由随机生成的位置索引重新计算删除迭代器，删除位置范围为`[0,N)`，`N`为当前容器元素个数，`N`随删除而递减
- 在原有的1200万128字节每元素的数据上随机删除600万次，重复测试50次，统一调用`erase`，之后还有64字节、32字节、16字节的600万次随机删除，详见excel表(无比对)，这里仅展示128字节的比对

|容器类型|平均耗时，含随机位置生成时间(ms)|
|-|-|
|container_plus|6320.9|
|std::vector|>3600000|
|std::deque|>3600000|
|std::list|>3600000|

## 注意事项
- *迭代器不保证稳定，在插入、删除后需重新获取*
- *不为线程安全，如需多线程操作，外部需加锁*
- *需要C++23及其以上的支持*

## *实现原理另见文档和源码*
