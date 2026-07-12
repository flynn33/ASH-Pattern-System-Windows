# Contributing to ASH Pattern System — Windows Development Migration

Thank you for your interest in contributing to the ASH Pattern System — Windows Development Migration project! This document provides guidelines and instructions for contributing.

## Code of Conduct

This project adheres to the [Contributor Covenant](https://www.contributor-covenant.org/version/2/1/code_of_conduct/). By participating, you are expected to uphold this code. Please report unacceptable behavior to the maintainers.

## How to Contribute

### Reporting Bugs

Before creating bug reports, please check existing issues to see if a similar issue has already been reported. When creating a new bug report, include:

- A clear and descriptive title
- Steps to reproduce the bug
- Expected behavior
- Actual behavior
- Environment details (OS, compiler version, etc.)

### Suggesting Features

Feature suggestions are welcome! Please create an issue with:

- A clear and descriptive title
- A detailed description of the feature
- Use cases for the feature
- Any relevant examples or references

### Pull Requests

Pull requests are welcome! Before submitting a pull request:

1. Fork the repository
2. Create a new branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Run tests to ensure everything works
5. Commit your changes with a clear message
6. Push to your fork (`git push origin feature/your-feature`)
7. Submit a pull request

### Style Guide

- Use modern C++ (C++17/20)
- Follow the existing code style
- Add comments where necessary
- Write clear and concise commit messages

## Development Setup

### Prerequisites

- Visual Studio 2022 or later
- CMake 3.15 or later
- Git
- Modern C++ compiler (C++17/20)

### Clone the Repository

```bash
git clone https://github.com/your-org/ash-pattern-system-windows-migration.git
cd ash-pattern-system-windows-migration
```

### Build the Project

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Testing

Run tests to ensure your changes don't break existing functionality:

```bash
ctest --output-on-failure
```

## Documentation

Documentation is important! Please update the documentation when making changes:

- Update the README.md if necessary
- Add examples for new features
- Update the CHANGELOG.md with your changes

## Questions?

If you have any questions, please feel free to open an issue or contact the maintainers.

Thank you for contributing to ASH Pattern System — Windows Development Migration!
