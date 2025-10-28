# Standalone GPIO Trigger Test

This standalone program tests GPIO trigger performance **without any Camera SDK dependencies**. Use this to measure pure GPIO overhead and compare against SDK-based triggering.

## Purpose

This test helps you:
- Measure pure GPIO trigger performance
- Identify if SDK operations are adding latency
- Benchmark the theoretical maximum trigger rate
- Test hardware wiring without camera connection

## Files

- `gpio_only_test.cpp` - Standalone GPIO trigger test
- `Makefile.gpio_test` - Build configuration

## Requirements

- Raspberry Pi (tested on Pi 5)
- libgpiod library: `sudo apt-get install libgpiod-dev`
- C++17 compiler (g++)

## Build

```bash
make -f Makefile.gpio_test
```

Or manually:
```bash
g++ -std=c++17 -Wall -O2 -o gpio_only_test gpio_only_test.cpp -lgpiod
```

## Run

```bash
./gpio_only_test
```

## Configuration

Default settings (edit in `gpio_only_test.cpp`):
- **GPIO Pin**: 12 (Physical pin 32 on gpiochip4)
- **Press duration**: 20ms
- **Cycle delay**: 300ms (camera processing + SD card save time)
- **Total triggers**: 30

## What It Does

1. Initializes GPIO pin 12 as output
2. Triggers GPIO HIGH for 20ms, then LOW
3. Waits 300ms for camera processing and SD card save
4. Repeats 30 times
5. Reports total time and average FPS

## Output

The test creates a timestamped log file:
```
gpio_standalone_test_YYYYMMDD_HHMMSS.log
```

Example output:
```
=== Test Complete ===
Total triggers: 30
Total time: 9.62 seconds
Average speed: 3.12 fps
Average cycle time: 321 ms

Log saved to: gpio_standalone_test_20251028_143022.log
```

## Comparison with SDK-based Tests

| Test | Description | Expected FPS |
|------|-------------|--------------|
| `gpio_only_test` | Pure GPIO, no SDK | ~3.1 fps |
| Option 9 in CLI | GPIO trigger with SDK connection | ~3.1 fps |
| Option 8 in CLI | GPIO + focus control + SDK | ~2.5 fps |

If the standalone test and Option 9 show similar performance (~3.1 fps), the SDK connection overhead is minimal.

## Troubleshooting

**"Failed to open GPIO chip"**
- Install libgpiod: `sudo apt-get install libgpiod-dev`
- Run with sudo if permission denied: `sudo ./gpio_only_test`

**"Failed to request GPIO line as output"**
- GPIO may be in use by another process
- Check if the main CLI app is running

## Clean Up

```bash
make -f Makefile.gpio_test clean
```
