#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <functional>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

namespace ash {
namespace migration {

// Smart pointer wrapper
class SharedPtrWrapper {
public:
    template<typename T>
    SharedPtrWrapper() = default;
    
    template<typename T>
    explicit SharedPtrWrapper(std::shared_ptr<T> ptr) : ptr_(std::move(ptr)) {}
    
    template<typename T>
    ~SharedPtrWrapper() = default;
    
    // Copy constructor
    SharedPtrWrapper(const SharedPtrWrapper& other) : ptr_(other.ptr_) {}
    
    // Move constructor
    SharedPtrWrapper(SmartPtrWrapper&& other) noexcept : ptr_(std::move(other.ptr_)) {}
    
    // Copy assignment
    SharedPtrWrapper& operator=(const SharedPtrWrapper& other) {
        if (this != &other) {
            ptr_ = other.ptr_;
        }
        return *this;
    }
    
    // Move assignment
    SharedPtrWrapper& operator=(SharedPtrWrapper&& other) noexcept {
        if (this != &other) {
            ptr_ = std::move(other.ptr_);
        }
        return *this;
    }
    
    // Get pointer
    template<typename T>
    std::shared_ptr<T> get() const { return ptr_; }
    
    // Dereference
    template<typename T>
    T& operator*() const { return *ptr_; }
    
    // Pointer access
    template<typename T>
    T* operator->() const { return ptr_.get(); }
    
private:
    std::shared_ptr<void> ptr_;
};

// Optional wrapper
class OptionalWrapper {
public:
    template<typename T>
    OptionalWrapper() = default;
    
    template<typename T>
    explicit OptionalWrapper(std::optional<T> opt) : opt_(std::move(opt)) {}
    
    template<typename T>
    ~OptionalWrapper() = default;
    
    // Copy constructor
    OptionalWrapper(const OptionalWrapper& other) : opt_(other.opt_) {}
    
    // Move constructor
    OptionalWrapper(OptionalWrapper&& other) noexcept : opt_(std::move(other.opt_)) {}
    
    // Copy assignment
    OptionalWrapper& operator=(const OptionalWrapper& other) {
        if (this != &other) {
            opt_ = other.opt_;
        }
        return *this;
    }
    
    // Move assignment
    OptionalWrapper& operator=(OptionalWrapper&& other) noexcept {
        if (this != &other) {
            opt_ = std::move(other.opt_);
        }
        return *this;
    }
    
    // Check if has value
    template<typename T>
    bool has_value() const { return opt_.has_value(); }
    
    // Get value or default
    template<typename T, typename Default = T>
    Default value_or(Default&& default_val) const {
        if (opt_.has_value()) {
            return *opt_;
        }
        return default_val;
    }
    
private:
    std::optional<void> opt_;
};

// Variant wrapper
class VariantWrapper {
public:
    template<typename... Ts>
    VariantWrapper() = default;
    
    template<typename T, typename... Ts>
    explicit VariantWrapper(std::variant<T, Ts...> var) : var_(std::move(var)) {}
    
    template<typename... Ts>
    ~VariantWrapper() = default;
    
    // Copy constructor
    VariantWrapper(const VariantWrapper& other) : var_(other.var_) {}
    
    // Move constructor
    VariantWrapper(VariantWrapper&& other) noexcept : var_(std::move(other.var_)) {}
    
    // Copy assignment
    VariantWrapper& operator=(const VariantWrapper& other) {
        if (this != &other) {
            var_ = other.var_;
        }
        return *this;
    }
    
    // Move assignment
    VariantWrapper& operator=(VariantWrapper&& other) noexcept {
        if (this != &other) {
            var_ = std::move(other.var_);
        }
        return *this;
    }
    
    // Check type
    template<typename T>
    bool holds_alternative() const { return std::holds_alternative<T>(var_); }
    
    // Get value
    template<typename T>
    T& get() { return std::get<T>(var_); }
    
    template<typename T>
    const T& get() const { return std::get<T>(var_); }
    
private:
    std::variant<void, void> var_;
};

// Function wrapper
class FunctionWrapper {
public:
    using Callback = std::function<void()>;
    
    FunctionWrapper() = default;
    
    explicit FunctionWrapper(Callback callback) : callback_(std::move(callback)) {}
    
    ~FunctionWrapper() = default;
    
    // Copy constructor
    FunctionWrapper(const FunctionWrapper& other) : callback_(other.callback_) {}
    
    // Move constructor
    FunctionWrapper(FunctionWrapper&& other) noexcept : callback_(std::move(other.callback_)) {}
    
    // Copy assignment
    FunctionWrapper& operator=(const FunctionWrapper& other) {
        if (this != &other) {
            callback_ = other.callback_;
        }
        return *this;
    }
    
    // Move assignment
    FunctionWrapper& operator=(FunctionWrapper&& other) noexcept {
        if (this != &other) {
            callback_ = std::move(other.callback_);
        }
        return *this;
    }
    
    // Call function
    void operator()() const { callback_(); }
    
private:
    Callback callback_;
};

// Lambda wrapper
class LambdaWrapper {
public:
    using Lambda = std::function<void()>;
    
    LambdaWrapper() = default;
    
    explicit LambdaWrapper(Lambda lambda) : lambda_(std::move(lambda)) {}
    
    ~LambdaWrapper() = default;
    
    // Copy constructor
    LambdaWrapper(const LambdaWrapper& other) : lambda_(other.lambda_) {}
    
    // Move constructor
    LambdaWrapper(LambdaWrapper&& other) noexcept : lambda_(std::move(other.lambda_)) {}
    
    // Copy assignment
    LambdaWrapper& operator=(const LambdaWrapper& other) {
        if (this != &other) {
            lambda_ = other.lambda_;
        }
        return *this;
    }
    
    // Move assignment
    LambdaWrapper& operator=(LambdaWrapper&& other) noexcept {
        if (this != &other) {
            lambda_ = std::move(other.lambda_);
        }
        return *this;
    }
    
    // Call lambda
    void operator()() const { lambda_(); }
    
private:
    Lambda lambda_;
};

} // namespace migration
} // namespace ash
