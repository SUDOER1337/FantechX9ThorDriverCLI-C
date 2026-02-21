# Contributing to Fantech X9 Thor Driver CLI

Thank you for your interest in contributing! This document provides guidelines for contributing to this C implementation.

## Development Setup

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd FantechX9ThorDriverCLI-c
   ```

2. **Install dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt install build-essential libusb-1.0-0-dev
   
   # Fedora/CentOS
   sudo dnf install gcc make libusb1-devel
   
   # Arch Linux
   sudo pacman -S base-devel libusb
   ```

3. **Build and test**
   ```bash
   make clean-install
   fantech-driver --help
   ```

## Code Style Guidelines

- **C99 Standard**: Use C99 features only
- **Indentation**: 4 spaces (no tabs)
- **Naming**: `snake_case` for functions, `PASCAL_CASE` for constants
- **Comments**: Explain complex logic, USB protocol details
- **Memory Management**: Always check allocations, free resources

## Contribution Areas

### High Priority
- [ ] Enhanced error reporting
- [ ] Complete preset system implementation
- [ ] Auto-detection of device changes

### Medium Priority
- [ ] TUI interface (textual-like)
- [ ] Systemd service integration
- [ ] Configuration validation

### Low Priority
- [ ] Additional lighting effects
- [ ] Profile switching shortcuts
- [ ] GUI configuration tool

## Submitting Changes

1. **Fork the repository**
2. **Create feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make changes**
   - Follow code style guidelines
   - Add tests if applicable
   - Update documentation

4. **Test thoroughly**
   ```bash
   make clean
   make test-compile
   make
   ```

5. **Commit changes**
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

6. **Push and create pull request**
   ```bash
   git push origin feature/your-feature-name
   ```

## Bug Reports

When reporting bugs, please include:
- Operating system and distribution
- libusb version
- Mouse firmware version (if known)
- Steps to reproduce
- Expected vs actual behavior
- Any error messages

## Feature Requests

Feature requests should:
- Be specific about the functionality
- Explain the use case
- Consider implementation complexity
- Suggest API/CLI design if applicable

## USB Protocol Contributions

When working with USB protocol:
- Document any new protocol discoveries
- Include reference to original Python behavior
- Test with multiple mouse firmware versions
- Preserve backward compatibility

## License

By contributing, you agree that your contributions will be licensed under the MIT License, matching this project's license.

## Questions?

Feel free to open an issue for any questions about contributing!
