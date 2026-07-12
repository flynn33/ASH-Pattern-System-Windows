// ASH Pattern System — Windows Development Migration — Modern Features Example
//
// This file demonstrates the use of modern C++ features in the ASH Pattern System.

#include "include/modern_features.hpp"
#include <iostream>

int main() {
    // Initialize modern features
    if (!ModernFeatures::Initialize()) {
        std::cerr << "Failed to initialize modern features." << std::endl;
        return 1;
    }

    // Smart pointer wrapper
    auto ptr = ash::migration::SharedPtrWrapper<std::shared_ptr<Window>>();
    ptr->SetHwnd(HWND_INVALID);

    // Optional wrapper
    auto opt = ash::migration::OptionalWrapper<std::optional<int>>();
    opt->Value(42);

    // Variant wrapper
    auto var = ash::migration::VariantWrapper<std::variant<int, std::string>>();
    var->Value(42);

    // Function wrapper
    auto func = ash::migration::FunctionWrapper([](int x) { return x * 2; });
    int result = func->Call(21);

    // Lambda wrapper
    auto lambda = ash::migration::LambdaWrapper([]() { std::cout << "Hello, World!" << std::endl; });
    lambda->Invoke();

    // Shutdown modern features
    ModernFeatures::Shutdown();

    return 0;
}
