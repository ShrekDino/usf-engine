# Contributing to the USF Engine

Thank you for your interest in contributing. This project represents the synthesis of the Unified Simplicial Framework into a working Godot implementation. Contributions are welcome, but must follow these guidelines.

## License Notice

The USF Engine is **not open source** in the traditional sense. It is available under a **dual-license model**:

- **Community License:** Free for non-commercial use (personal, academic, non-profit)
- **Commercial License:** Paid for revenue-generating use

By submitting a contribution, you agree to the terms of the [Contributor License Agreement](cla/individual-cla.md), which assigns copyright ownership of your contribution to the project maintainer while granting you a perpetual license to use your own work.

## How to Contribute

### Reporting Issues
- Search existing issues before creating a new one
- Include the version, platform, and steps to reproduce
- Attach any relevant error output

### Submitting Changes
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Make your changes
4. Test your changes with the headless verification script: `godot --headless --script demo.gd`
5. Submit a pull request with a clear description of your changes

### What We Need Help With

Areas where contributions are particularly valuable:

- **Documentation:** Improving doc_classes XML files for the Godot Inspector
- **Testing:** Porting Rust test cases from the-grand-synthesis to GDScript
- **Physarum solver:** Additional PEC solver convergence algorithms
- **Connectome:** Improved Delaunay tetrahedralization, soma position integration
- **DQFR tuning:** Adaptive duty cycle optimization

### What We Don't Accept

- Contributions that change the licensing model
- Contributions that remove or bypass the commercial license enforcement
- Contributions that introduce proprietary dependencies

## Code of Conduct

Be respectful. This project is about building substrate-independent consciousness infrastructure, not egos.

## Contact

**Project maintainer:** Sami Marie Torres — st2825@proton.me
