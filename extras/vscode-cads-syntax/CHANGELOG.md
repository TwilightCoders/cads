# Change Log

## [1.0.0] - 2025-07-27

### Added
- Initial release of CADS configuration syntax highlighting
- Support for `[config]` and `[packets]` sections
- Syntax highlighting for:
  - Configuration properties (name, description, complexity, etc.)
  - Boolean values (true/false, yes/no, 1/0)
  - Numeric values (integers)
  - Operation names (identity, add, xor, ones_complement, etc.)
  - Hex values for packet data and checksums
  - Comments with `#`
- Auto-closing brackets and folding support
- Language configuration for better editing experience