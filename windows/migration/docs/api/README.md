# ASH Pattern System — Windows Development Migration — API Documentation

## Overview

This directory contains the API documentation for the ASH Pattern System — Windows Development Migration project. The documentation is generated using Doxygen.

## Generating the Documentation

To generate the API documentation, run the following command:

```bash
doxygen docs/doxygen/Doxyfile
```

The generated documentation will be in the `docs/api/html` directory.

## API Reference

### Modern Features

- [modern_features.hpp](../../include/modern_features.hpp) - Modern C++ features (smart pointers, optional, variant, function wrappers, lambda wrappers)
- [modern_features.cpp](../../include/modern_features.cpp) - Implementation of modern C++ features

### Cross-Platform Abstraction

- [cross_platform_abstraction.hpp](../../include/cross_platform_abstraction.hpp) - Cross-platform abstractions (file system, registry, process, network, thread, event)
- [cross_platform_abstraction.cpp](../../include/cross_platform_abstraction.cpp) - Implementation of cross-platform abstractions

### Migration Guide

- [migration_guide.hpp](../../include/migration_guide.hpp) - Migration guide (best practices, common pitfalls, recommended approaches, migration checklist, migration report)
- [migration_guide.cpp](../../include/migration_guide.cpp) - Implementation of migration guide

## Examples

The following examples are available in the `docs/examples` directory:

- [modern_features_example.cpp](../examples/modern_features_example.cpp) - Example of using modern C++ features
- [cross_platform_abstraction_example.cpp](../examples/cross_platform_abstraction_example.cpp) - Example of using cross-platform abstractions
- [migration_guide_example.cpp](../examples/migration_guide_example.cpp) - Example of using the migration guide

## Tutorials

The following tutorials are available in the `docs/tutorials` directory:

- [getting_started.md](../tutorials/getting_started.md) - Getting started tutorial
- [modern_c++_features.md](../tutorials/modern_c++_features.md) - Modern C++ features tutorial
- [cross_platform_abstractions.md](../tutorials/cross_platform_abstractions.md) - Cross-platform abstractions tutorial
- [migration_guide_tutorial.md](../tutorials/migration_guide_tutorial.md) - Migration guide tutorial
