# CADS CLI Usage Guide

## Basic Usage

```bash
# Analyze packets from a file with 1-byte checksums
./cads --input packets.csv --complexity basic --checksum-size 1

# Use 2-byte checksums in little-endian format
./cads --input packets.json --complexity intermediate --checksum-size 2 --little-endian

# Advanced analysis with custom parameters
./cads --input data.hex --complexity advanced --max-fields 4 --checksum-size 4 --verbose
```

## Input File Formats

### CSV Format
```csv
description,packet_data,expected_checksum
"CH1","9c30010000000000","31"
"CH3","9c30030000000000","33"
"CH15","9c300f00000100","3e"
```

### JSON Format
```json
{
  "packets": [
    {
      "description": "CH1",
      "data": "9c30010000000000",
      "checksum": "31",
      "checksum_size": 1
    },
    {
      "description": "Multi-byte example",
      "data": "deadbeef",
      "checksum": "1234",
      "checksum_size": 2
    }
  ]
}
```

### HEX Format (Simple)
```
# Comments start with #
# Format: <hex_data>:<checksum>:<description>
9c30010000000000:31:CH1
9c30030000000000:33:CH3
9c300f00000100:3e:CH15
```

## Command Line Options

### Core Options
- `--input FILE` - Input file with test packets (required)
- `--complexity [basic|intermediate|advanced|all]` - Algorithm complexity level
- `--checksum-size N` - Checksum size in bytes (1-8, default: 1)
- `--max-fields N` - Maximum packet fields to combine (default: auto-detect)
- `--max-constants N` - Maximum constant value to test (1-65536, default: 256)

### Search Control Options
- `--early-exit` - Stop after finding the first working solution (fast discovery)
- `--max-solutions N` - Find at most N solutions, then stop (default: unlimited)
- `--1-solution` - Alias for `--early-exit` (find exactly one solution and exit)

### Output Options
- `--output FILE` - JSON output file for results
- `--verbose` - Detailed progress information
- `--progress-interval N` - Progress update frequency in seconds (default: 5)

### Data Format Options
- `--little-endian` - Interpret multi-byte checksums as little-endian (default: big-endian)

### Performance Options
- `--threads N` - Number of parallel threads (future feature)
- `--resume FILE` - Resume from checkpoint file

### Information Options
- `--help` - Show help message
- `--version` - Show version information
- `--list-algorithms` - List all available algorithms by complexity
- `--estimate-time` - Show time estimates without running

## Examples

### Basic Radio Protocol Analysis
```bash
# Quick discovery - find first working solution and exit
./cads --input gmrs_packets.csv --complexity basic --early-exit --verbose

# Find exactly one solution (alias for --early-exit)
./cads --input gmrs_packets.csv --complexity basic --1-solution

# Find up to 3 different solutions for comparison
./cads --input gmrs_packets.csv --complexity basic --max-solutions 3

# Exhaustive search - find ALL possible solutions (default behavior)
./cads --input gmrs_packets.csv --complexity basic --verbose
```

### Network Protocol Analysis  
```bash
# Analyze network packets with 2-byte checksums
./cads --input network_data.json --complexity intermediate --checksum-size 2 --little-endian
```

### Custom Binary Protocol
```bash
# Complex protocol with 4-byte checksums
./cads --input protocol.hex --complexity advanced --checksum-size 4 --max-fields 8 --output results.json
```

### Time Estimation
```bash
# Estimate how long analysis will take
./cads --input large_dataset.csv --complexity advanced --checksum-size 2 --estimate-time

# Output:
# Configuration Analysis:
#   Packet Count: 1,247 packets
#   Packet Size: 16 bytes average
#   Checksum Size: 2 bytes
#   Algorithm Count: 11 (advanced)
#   Estimated Combinations: ~2.4B
#   Estimated Time: 4h 23m (based on 153K tests/sec)
```

## Checksum Size Implications

### 1-byte checksums (default)
- Fast analysis (minutes to hours)
- 256 possible values
- Most common for simple protocols

### 2-byte checksums
- Moderate analysis time (hours to days)
- 65,536 possible values
- Common for network protocols

### 4-byte checksums
- Long analysis time (days to weeks)
- 4.3 billion possible values
- Used for robust error detection

### 8-byte checksums
- Very long analysis time (weeks to months)
- Extremely large search space
- Cryptographic-grade checksums

## Performance Impact of Early Exit

The `--early-exit` and `--max-solutions` options can dramatically reduce analysis time:

```bash
# Performance comparison for same dataset:

# Exhaustive search: Find ALL solutions (could take hours/days)
./cads --input packets.csv --complexity intermediate
# Result: Found 23 different working solutions in 4.2 hours

# Early exit: Find FIRST solution (seconds to minutes)  
./cads --input packets.csv --complexity intermediate --early-exit
# Result: Found 1 solution in 47 seconds (99% time savings!)

# Controlled search: Find a few solutions for comparison
./cads --input packets.csv --complexity intermediate --max-solutions 5
# Result: Found 5 solutions in 3.8 minutes (95% time savings)
```

### When to Use Each Mode

**Early Exit (`--early-exit` or `--1-solution`):**
- ✅ Quick protocol reverse engineering
- ✅ Initial analysis of unknown protocols  
- ✅ When you just need "a" solution, not "all" solutions
- ✅ Time-constrained analysis

**Limited Solutions (`--max-solutions N`):**
- ✅ Compare multiple algorithm approaches
- ✅ Find backup/alternative solutions
- ✅ Validation with multiple approaches

**Exhaustive Search (default):**
- ✅ Academic research requiring completeness
- ✅ Security analysis needing all attack vectors
- ✅ Algorithm development and testing

## Resume and Checkpointing

```bash
# Long-running analysis with automatic checkpointing
./cads --input large_data.csv --complexity advanced --output results.json

# If interrupted, resume from last checkpoint
./cads --resume cads_checkpoint_20240727_143022.dat --output results.json
```

Checkpoints are automatically created every 10 minutes during analysis and when the program is interrupted (Ctrl+C).