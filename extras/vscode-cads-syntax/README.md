# CADS Configuration Syntax Extension

This VSCode extension provides syntax highlighting for CADS (Checksum Algorithm Discovery System) configuration files with the `.cads` extension.

## Features

- **Syntax highlighting** for .cads configuration files
- **Section highlighting** for `[config]` and `[packets]` sections
- **Property highlighting** for configuration keys and values
- **Operation highlighting** for algorithm operation names
- **Hex value highlighting** for packet data and checksums
- **Comment support** with `#` line comments
- **Auto-closing brackets** and folding support

## Supported Syntax

### Configuration Section
```cads
[config]
name = MXT275 Quick Verification
description = Real MXT275 algorithm discovery
complexity = advanced
max_fields = 4
max_constants = 209
early_exit = true
max_solutions = 1
progress_interval = 100
verbose = true
operations = ones_complement,const_add,xor
```

### Packets Section
```cads
[packets]
# Packet data format: packet_hex checksum_hex [description]
9c30010000000000 31 CH1
9c30030000000000 33 CH3
9c300f0000010000 3e CH15
9c30020109000000 3a CH2+CTCSS09
```

## Installation

1. Copy the `vscode-cads-syntax` folder to your VSCode extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS**: `~/.vscode/extensions/`
   - **Linux**: `~/.vscode/extensions/`

2. Restart VSCode

3. Open any `.cads` file to see syntax highlighting

## Development

To modify this extension:

1. Edit `syntaxes/cads.tmLanguage.json` for syntax rules
2. Edit `language-configuration.json` for language features
3. Reload VSCode window to see changes

## License

MIT License - Part of the CADS project by TwilightCoders.