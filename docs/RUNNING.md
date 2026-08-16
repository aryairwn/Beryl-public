# Running Beryl V1

## Full Node

Start the Beryl daemon:

```bash
cd src
./beryld
```

The daemon must be running before using RPC commands through `beryl-cli`.

## CLI

With the daemon running:

```bash
cd src
./beryl-cli
```

The CLI communicates with the Beryl daemon.

## Wallet

Start the wallet:

```bash
cd src
./beryl-wallet
```

Wallet data and private key material must remain local.

Never commit wallet files, seed phrases, or private keys to Git.

## Miner

Beryl V1 uses YesPower Proof of Work.

Start the miner by specifying the number of mining threads:

```bash
cd src
./beryl-miner <threads>
```

Example:

```bash
./beryl-miner 4
```
