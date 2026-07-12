# ASH Pattern System — Windows Development Migration — Modern C++ Features Tutorial

## Overview

This tutorial provides a comprehensive introduction to modern C++ features in the ASH Pattern System — Windows Development Migration project. It covers smart pointers, optional, variant, function wrappers, and lambda wrappers.

## Smart Pointers

The ASH Pattern System provides smart pointer wrappers for Windows development:

### Shared Pointer Wrapper

```cpp
#include "include/modern_features.hpp"

// Create a shared pointer wrapper
auto ptr = ash::migration::SharedPtrWrapper<std::shared_ptr<Window>>();

// Get the underlying pointer
T* get() { return m_ptr.get(); }

// Dereference the pointer
T& operator*() { return *m_ptr; }

// Check if the pointer is valid
bool operator!() const { return !m_ptr; }
```

### Example

```cpp
#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Create a shared pointer wrapper
    auto ptr = ash::migration::SharedPtrWrapper<std::shared_ptr<int>>();
    
    // Create a new integer
    ptr = ash::migration::SharedPtrWrapper<std::shared_ptr<int>>(std::make_shared<int>(42));
    
    // Dereference the pointer
    std::cout << *ptr << std::endl; // Output: 42
    
    return 0;
}
```

## Optional

The ASH Pattern System provides optional wrappers for Windows development:

### Optional Wrapper

```cpp
#include "include/modern_features.hpp"

// Create an optional wrapper
auto opt = ash::migration::OptionalWrapper<std::optional<int>>();

// Check if the optional has a value
bool has_value() const { return m_opt.has_value(); }

// Get the value or a default value
T* value_or(T&& default_val) { return m_opt.value_or(std::forward<T>(default_val)); }

// Dereference the optional
const T& operator*() const & { return *m_opt; }
```

### Example

```cpp
#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Create an optional wrapper with a value
    auto opt = ash::migration::OptionalWrapper<std::optional<int>>(std::optional<int>(42));
    
    // Check if the optional has a value
    std::cout << opt.has_value() << std::endl; // Output: 1
    
    // Get the value or a default value
    std::cout << *opt.value_or(0) << std::endl; // Output: 42
    
    return 0;
}
```

## Variant

The ASH Pattern System provides variant wrappers for Windows development:

### Variant Wrapper

```cpp
#include "include/modern_features.hpp"

// Create a variant wrapper
auto var = ash::migration::VariantWrapper<std::variant<int, std::string>>();

// Check if the variant holds a specific type
template<typename T>
bool holds_alternative() const { return m_var.holds_alternative<T>(); }

// Get the value of a specific type
template<typename T>
T& get() { return std::get<T>(m_var); }
```

### Example

```cpp
#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Create a variant wrapper with an integer
    auto var = ash::migration::VariantWrapper<std::variant<int, std::string>>(std::variant<int, std::string>(42));
    
    // Check if the variant holds an integer
    std::cout << var.holds_alternative<int>() << std::endl; // Output: 1
    
    // Get the value of the integer
    std::cout << var.get<int>() << std::endl; // Output: 42
    
    return 0;
}
```

## Function Wrappers

The ASH Pattern System provides function wrappers for Windows development:

### Function Wrapper

```cpp
#include "include/modern_features.hpp"

// Create a function wrapper
auto func = ash::migration::FunctionWrapper<int(int)>([](int x) { return x * 2; });

// Call the function
int result = func(21); // Returns 42
```

### Example

```cpp
#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Create a function wrapper
    auto func = ash::migration::FunctionWrapper<int(int)>([](int x) { return x * 2; });
    
    // Call the function
    std::cout << func(21) << std::endl; // Output: 42
    
    return 0;
}
```

## Lambda Wrappers

The ASH Pattern System provides lambda wrappers for Windows development:

### Lambda Wrapper

```cpp
#include "include/modern_features.hpp"

// Create a lambda wrapper
auto lambda = ash::migration::LambdaWrapper([]() { std::cout << "Hello, World!" << std::endl; });

// Call the lambda
lambda(); // Outputs: Hello, World!
```

### Example

```cpp
#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Create a lambda wrapper
    auto lambda = ash::migration::LambdaWrapper([]() { std::cout << "Hello, World!" << std::endl; });
    
    // Call the lambda
    lambda(); // Outputs: Hello, World!
    
    return 0;
}
```

## Next Steps

- [Getting Started Tutorial](getting_started.md) - Getting started tutorial
- [Cross-Platform Abstractions Tutorial](cross_platform_abstractions.md) - Cross-platform abstractions tutorial
- [Migration Guide Tutorial](migration_guide_tutorial.md) - Migration guide tutorial
