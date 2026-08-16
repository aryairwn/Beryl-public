# Security

## Wallet Data

Never upload or commit:

- `wallet.dat`
- Seed phrases
- Private keys
- Secret key files
- Wallet backups
- Personal credentials

The repository `.gitignore` excludes common wallet and secret-file patterns.

## Seed Phrase

A Beryl wallet seed phrase is sensitive secret material.

Never share a seed phrase in chat, GitHub, screenshots, logs, or public repositories.

## Public Repository

Before pushing changes, inspect the staged files:

```bash
git diff --cached --name-only
```

Verify that no wallet or secret files are included.

## Important

Anyone who obtains a wallet private key or seed phrase may be able to control the associated funds.

Keep wallet backups offline and secure.
