// MIT License
// 
// Copyright (c) 2023 Leo Heinsaar
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <string_view>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <random>
#include <atomic>
#include <ctime>

namespace zen {

///////////////////////////////////////////////////////////////////////////////////////////// USEFUL MISC

// Repeats a string patterns.
// This is the symmetrical complement of repeat(int, str).
// Example: repeat("*", 10);
// Result:  "**********"
zen::string repeat(const std::string_view s, const int n) {
    std::string result;
    for (int i = 0; i < n; i++) {
        result += s;
    }
    return result;
}

// This is the symmetrical complement of repeat(str, int).
// Repeats a string patterns.
// Example: repeat(10, "*");
// Result:  "**********"
zen::string repeat(const int n, const std::string_view s) {
    std::string result;
    for (int i = 0; i < n; i++) {
        result += s;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////////////////// MAIN UTILITIES

// Example: random_int();
// Result: A random integer between [min, max)
template<class T = int>
T random_int(const T min = 0, const T max = 10) {
    // Reasons why the std::random_device and the std::mt19937 are 'static' below:
    // ---------------------------------------------------------------------------------------------------------------
    // 1. Initialization Efficiency:
    // Random devices and generators often involve some computational cost due to entropy gathering, seeding, generator
    // initialization and good old algorithmic complexity. By declaring them as static, they are initialized only once,
    // the first time the function is called. Subsequent calls to to this function reuse the existing instances, avoiding the overhead.
    // ---------------------------------------------------------------------------------------------------------------
    // 2. State Preservation:
    // Random number generators like std::mt19937 maintain an internal state that evolves as numbers are generated.
    // This state determines the sequence of random numbers produced. By making the std::mt19937 object static,
    // the state is preserved across calls to this function, ensuring a proper random sequence. If the std::mt19937
    // object were reinitialized on every call, it might lead to repeated or patterned sequences, undermining the randomness.
    // ---------------------------------------------------------------------------------------------------------------
    // 3. Thread Safety Considerations:
    // Declaring these variables as static within a function means they are shared across all calls to that function
    // within the program, regardless of where it's called from. This could potentially raise thread-safety issues if the
    // function is called simultaneously from multiple threads. However, in most common usage scenarios where thread
    // safety is not a concern, using static variables for this purpose is fine.
    // ---------------------------------------------------------------------------------------------------------------
    // 4. Resource Management
    // Since the std::random_device and the std::mt19937 object are static, they are not destroyed when the function returns,
    // but rather when the program ends. This ensures that the same random device and generator instances are reused
    // throughout the lifetime of the program, optimizing resource management.
    // ---------------------------------------------------------------------------------------------------------------
    // 5. Avoiding Repetition
    // If we didn't make these objects static, and the random number generator was reseeded with the same or similar
    // seeds (which might happen if the function is called in quick succession), you might get the same or similar random
    // numbers in different calls. Making these static ensures a more varied and truly random sequence.
    // ---------------------------------------------------------------------------------------------------------------
    // The 'static' below is for reasons outlined above.
    static std::random_device        rd;
    static std::mt19937              gen(rd());
    std::uniform_int_distribution<T> dis(min, max);
    return dis(gen);
}

// Very often all we want is a dead simple way of quickly
// generating a container filled with some random numbers.
// Example: std::vector<int> v;
//          zen::generate_random(v);
// Result: A vector of size 10 with random integers between [min, max)
template<class Iterable>
void generate_random(Iterable& c, int size = 10) // TODO: Generalize & test with all containers before Kaizen 1.0.0 release
{
    ZEN_STATIC_ASSERT(zen::is_iterable_v< Iterable>, "TEMPLATE PARAMETER EXPECTED TO BE Iterable, BUT IS NOT");
    ZEN_STATIC_ASSERT(zen::is_resizable_v<Iterable>, "TEMPLATE PARAMETER EXPECTED TO BE Iterable, BUT IS NOT");

    if (std::empty(c))
        c.resize(size);

    std::generate(std::begin(c), std::end(c), [&]() { return random_int(10, 99); });
}

// Over the years it has become clear that the standard member
// function empty() that lacks an 'is_' prefix is confusing to
// non-familiar users due to its ambiguity as a noun and a verb.
// Example: zen::is_empty(c); // c is any iterable container
template<class HasEmpty>
bool is_empty(const HasEmpty& c)
{
    ZEN_STATIC_ASSERT(zen::has_empty_v<HasEmpty>, "TEMPLATE PARAMETER EXPECTED TO HAVE empty(), BUT DOES NOT");
    return c.empty();
}

// TODO: Think of a way to use the Addable concept in addition
// to Iterable that will not make the resulting code too ugly.
template<class Iterable>
auto sum(const Iterable& c)
{
    ZEN_STATIC_ASSERT(is_iterable_v<Iterable>,         "TEMPLATE PARAMETER EXPECTED TO BE Iterable, BUT IS NOT");
    ZEN_STATIC_ASSERT(is_addable_v<decltype(*std::begin(c))>, "ELEMENT TYPE EXPECTED TO BE Addable, BUT IS NOT");

    if (c.empty()) {
        return decltype(*std::begin(c)){}; // zero-initialized value for empty containers
    }

    // By initializing 'sum' to the first element of the collection and not just the tempting 0,
    // this function makes fewer assumptions about the type it's working with, thereby making
    // this function more robust and generic since we're dealing with arbitrary addable types
    // (which could be complex numbers, matrices, etc.).
    auto sum = *std::begin(c);
    for (auto it = std::next(std::begin(c)); it != std::end(c); ++it) {
        sum += *it;
    }

    return sum;
}

///////////////////////////////////////////////////////////////////////////////////////////// LPS (Log, Print, String)
// 
// Printing and logging in Kaizen follows the LPS principle of textual visualization.
// The LPS principle: from string to print to log. This means that:
// 1. to_string() - is the transformation of an object into a string
// 2. print()     - uses to_string() to output the object (as a string)
// 3. log()       - uses print() and adds any formatting, new lines at the end, etc.

// ------------------------------------------------------------------------------------------ stringify

// Converts most of the widely used data types to a string.
// Example: std::vector<int> v = {1, 3, 3};
// Example: to_string(vec) Result: [1, 2, 3]
// Example: to_string(42)  Result: "42"
template<class T>
zen::string to_string(const T& x) {
    std::stringstream ss;
    
    // First check for string-likeness so that zen::pring("abc") prints "abc"
    // and not [a, b, c] as a result of considering strings as iterable below
    if constexpr (is_string_like<T>()) {
        return x;
    } else if constexpr (is_iterable_v<T>) {
        ss << "[";
        auto it = std::begin(x);
        if (it != std::end(x)) {
            ss << to_string(*it++);       // recursive call to handle nested iterables
        }
        for (; it != std::end(x); ++it) {
            ss << ", " << to_string(*it); // recursive call to handle nested iterables
        }
        ss << "]";
    } else { // not iterable, single item
        ss << x;
    }
    return ss.str();
}

// Recursive variadic template to handle multiple arguments
template<class T, class... Args>
inline zen::string to_string(const T& x, const Args&... args) {
    return to_string(x) + " " + to_string(args...);
}
// Base case for the recursive calls
inline zen::string to_string() { return ""; }

// ------------------------------------------------------------------------------------------ print

// Function to handle individual item printing
template <class T>
void print(const T& x)
{
    std::cout << to_string(x);
}

// Generic, almost Python-like print(). Works like this:
// print("Hello", "World", vec, 42); // Output: Hello World [1, 2, 3] 42
// print("Hello", "World", 24, vec); // Output: Hello World 24 [1, 2, 3]
// print("Hello", vec, 42, "World"); // Output: Hello [1, 2, 3] 42 World
template <class T, class... Args>
void print(T x, Args... args) {
    print(x);
    if constexpr (sizeof...(args) != 0) {
        std::cout << " ";
    }
    print(args...);
}
// Base case for the recursive calls
inline void print() {}

// ------------------------------------------------------------------------------------------ log

// Handles logging a single item
template <class T>
void log(const T& x)
{
    print(x, '\n');
}

// Generic, almost Python-like log(). Works similar to the print() function but adds std::endl
template <class T, class... Args>
void log(T x, Args... args) {
    print(x, args...);
    std::cout << std::endl;
}
// Base case for the recursive calls
inline void log() {}

} // namespace zen