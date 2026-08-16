# Beryl

Beryl is an open-source blockchain project.

**Ticker:** BER  
**Version:** V1.0

## Overview

Beryl is a standalone blockchain implementation written primarily in C++.

The public repository contains the Beryl V1 source code, including:

- Full node daemon
- Command-line interface
- Wallet
- Miner
- Transaction and UTXO handling
- Proof-of-Work components
- YesPower integration
- Falcon-512 cryptographic integration
- BLAKE3 integration
- Contract / virtual-machine components
- Blockchain validation and consensus components

## Beryl V1

| Parameter | Value |
|---|---|
| Coin | Beryl |
| Ticker | BER |
| Block time | 6 seconds |
| Mining | Proof of Work |
| PoW algorithm | YesPower |
| Signature | Falcon-512 |
| Smallest unit | 0.00000001 BER |
| Initial block reward | 40 BER |

Beryl V1 uses Falcon-512 as its post-quantum signature system.

## Repository Structure

```text
Beryl-public/
├── assets/
├── docs/
└── src/
    ├── beryld.cpp
    ├── beryl-cli.cpp
    ├── beryl-miner.cpp
    ├── beryl-wallet.cpp
    ├── wallet.cpp
    ├── transaction.cpp
    ├── validation.cpp
    ├── consensus.cpp
    ├── pow.cpp
    ├── reward.cpp
    ├── storage.cpp
    ├── yespower/
    └── crypto/
        ├── BLAKE3/
        └── falcon/
