# XahauClient vs XRPLClient - protocol deltas

Snapshot against **Xahau mainnet** `2026.6.21-release+3350` / `server_definitions` (2026-07-19).

## Signing

| Item | XRPLClient | XahauClient |
|------|------------|-------------|
| Prefix single-sign | `STX\0` | same |
| Prefix multi-sign | `SMT\0` + accountID | same |
| Curves | Ed25519 + secp256k1 | same |
| **NetworkID in signed blob** | omit (main/test/dev) | **required** `>= 1025` |
| Mainnet NetworkID | 0 (field absent) | **21337** |
| Testnet NetworkID | 1 (field absent) | **21338** |
| Native fee unit | XRP drops | **XAH** drops |

`TxCommon.networkId` + `bagSetCommon()` emit field `NetworkID` (type=2, field=1, header `0x21`) when `networkId >= 1025`.

## Type codes that differ (selected)

| Type | XRPL code | Xahau code |
|------|-----------|------------|
| SetHook | - | **22** |
| URITokenMint... | - | **45-49** |
| NFTokenModify | 61 | **70** (catalog code only - fill/sign removed; prefer URIToken) |
| XChain* | 41-48 | **50-57** |
| DIDSet/Delete | 49/50 | **58/59** |
| **OracleSet/Delete** | **51/52** | **60/61** |
| MPT / Credential / Domain | 54-63 | **63-72** (shifted) |
| Invoke | - | **99** |
| ClaimReward | - | **98** |
| Import | - | **97** |
| Remit | - | **95** |
| CronSet | - | **93** |
| SetRemarks | - | **94** |

## Oracle field codes (Xahau)

| Field | Type | nth |
|-------|------|-----|
| LastUpdateTime | UInt32 | 15 |
| OracleDocumentID | UInt32 | 51 |
| AssetPrice | UInt64 | 23 |
| Scale | UInt8 | 4 |
| AssetClass | Blob | **29** |
| Provider | Blob | **30** |
| PriceDataSeries | STArray | 24 |
| BaseAsset / QuoteAsset | Currency (26) | 1 / 2 |

## Live on Xahau mainnet (high level)

Hooks, URIToken, Remit, Import, BalanceRewards/ClaimReward, Cron, Remarks, Touch, Clawback, DeepFreeze, PaychanAndEscrowForTokens, **PriceOracle**, expanded multi-sign, deposit auth, ...

## Not live / catalog-only (no fill/sign in `src`)

AMM (+AMMClawback), NFToken*, DID/Credential/MPT/PermissionedDomain, XChain*, Vault/Loan/Batch/DelegateSet - **catalog `TxType` codes only** after the EncodeOnly cut (see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md)). NamedHooks / HooksUpdate2 / HookOnV2 may appear in a release build without being enabled on public nets.

## Arduino paths (Xahau-native examples)

- `examples/tx/DecentralizedStorage/OracleSet/OracleSet.ino` (code 60)
- `examples/tx/Payments/Remit/Remit.ino` (code 95)
- `examples/tx/Other/CronSet/CronSet.ino` (code 93)
- `examples/tx/Other/ClaimReward/ClaimReward.ino` (code 98)
- `examples/tx/Other/SetHook/SetHook.ino` / `Invoke` (codes 22 / 99)
- `examples/tx/Other/Import/Import.ino` (code 97; encode; live needs xPOP)
- `examples/tx/Tokens/URITokenMint/URITokenMint.ino` (code 45)
