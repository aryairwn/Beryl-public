# Building Beryl V1

## Requirements

Beryl V1 is primarily written in C++.

A C++ compiler, Make, and the required system development libraries are required.

## Build

From the repository root:

```bash
cd src
make
```

The build produces the Beryl V1 executables:

- `beryld`
- `beryl-cli`
- `beryl-wallet`
- `beryl-miner`

Build output is intentionally excluded from Git by `.gitignore`.

## Android / Termux

Beryl can be built in an Android Termux environment using an ARM64-compatible C++ toolchain and the required dependencies.

The exact dependencies may vary depending on the Termux environment.

## Clean Build

To remove local build output:

```bash
cd src
rm -rf build
```

Then rebuild:

```bash
cd src
make
```
