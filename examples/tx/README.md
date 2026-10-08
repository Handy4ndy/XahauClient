# XahauClient transaction examples

Xahau **mainnet/testnet** live tree only. Non-Xahau / amendment-absent
sketches archived to a maintainer archive.

Grouped to match **Xahau** transaction feature areas:

> [Transaction Types](https://xahau.network/docs/protocol-reference/transactions/transaction-types/)

## Layout

```text
examples/tx/
  Accounts/               # 6 sketches
  Payments/               # 12 sketches (incl. Remit)
  Tokens/                 # 7 sketches (URIToken market + TrustSet/Clawback)
  DEX/                    # 2 sketches (OfferCreate/Cancel)
  DecentralizedStorage/   # 2 sketches (Oracle*)
  Other/                  # 6 sketches (ClaimReward/SetRemarks/CronSet/Invoke/SetHook/Import)
```

Arduino IDE: **File -> Examples -> XahauClient -> tx -> <Group> -> <TxName>**

## How to use

1. Open a sketch under `examples/tx/<Group>/<Name>/`.
2. Edit **CONFIG**: `WIFI_SSID`, `WIFI_PASSWORD`, `XAHAU_SEED`, `XAHAU_NETWORK`, `DO_SUBMIT`.
3. Replace destinations / ledger IDs / amounts for your scenario.
4. ESP32 + WiFi + [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) for live submit.

```cpp
const char* XAHAU_SEED = "sEd...";   // same idea as Wallet.from_seed
const int   XAHAU_NETWORK = 1;    // 0 mainnet | 1 testnet (index 2+ also maps to testnet; no public Devnet preset)
```

## Groups

### Accounts (6)

Account options, keys, tickets, multi-sign, deposit auth.

| Example | Path |
|---------|------|
| `AccountSet` | `tx/Accounts/AccountSet/` |
| `AccountDelete` | `tx/Accounts/AccountDelete/` |
| `SetRegularKey` | `tx/Accounts/SetRegularKey/` |
| `SignerListSet` | `tx/Accounts/SignerListSet/` |
| `TicketCreate` | `tx/Accounts/TicketCreate/` |
| `DepositPreauth` | `tx/Accounts/DepositPreauth/` |

### Payments (12)

Payment, Escrow, Checks, Payment Channels, Remit.

| Example | Path |
|---------|------|
| `Payment` | `tx/Payments/Payment/` |
| `Remit` | `tx/Payments/Remit/` |
| `EscrowCreate` | `tx/Payments/EscrowCreate/` |
| `EscrowFinish` | `tx/Payments/EscrowFinish/` |
| `EscrowCancel` | `tx/Payments/EscrowCancel/` |
| `CheckCreate` | `tx/Payments/CheckCreate/` |
| `CheckCash` | `tx/Payments/CheckCash/` |
| `CheckCancel` | `tx/Payments/CheckCancel/` |
| `PaymentChannelCreate` | `tx/Payments/PaymentChannelCreate/` |
| `PaymentChannelFund` | `tx/Payments/PaymentChannelFund/` |
| `PaymentChannelClaim` | `tx/Payments/PaymentChannelClaim/` |
| `PaymentMultiSign` | `tx/Payments/PaymentMultiSign/` |

### Tokens (7)

Trust lines, clawback, **URIToken market** (preferred NFT on Xahau).

| Example | Path |
|---------|------|
| `TrustSet` | `tx/Tokens/TrustSet/` |
| `Clawback` | `tx/Tokens/Clawback/` |
| `URITokenMint` | `tx/Tokens/URITokenMint/` |
| `URITokenBurn` | `tx/Tokens/URITokenBurn/` |
| `URITokenBuy` | `tx/Tokens/URITokenBuy/` |
| `URITokenCreateSellOffer` | `tx/Tokens/URITokenCreateSellOffer/` |
| `URITokenCancelSellOffer` | `tx/Tokens/URITokenCancelSellOffer/` |

### DEX (2)

Classic offers (order book).

| Example | Path |
|---------|------|
| `OfferCreate` | `tx/DEX/OfferCreate/` |
| `OfferCancel` | `tx/DEX/OfferCancel/` |

### Decentralized Storage (2)

Price Oracles (`PriceOracle` amendment enabled on mainnet/testnet).

| Example | Path |
|---------|------|
| `OracleSet` | `tx/DecentralizedStorage/OracleSet/` |
| `OracleDelete` | `tx/DecentralizedStorage/OracleDelete/` |

### Other (6)

Xahau-native ClaimReward / SetRemarks / CronSet / Invoke / SetHook / Import.

| Example | Path |
|---------|------|
| `ClaimReward` | `tx/Other/ClaimReward/` |
| `SetRemarks` | `tx/Other/SetRemarks/` |
| `CronSet` | `tx/Other/CronSet/` |
| `Invoke` | `tx/Other/Invoke/` |
| `SetHook` | `tx/Other/SetHook/` |
| `Import` | `tx/Other/Import/` (encode; live needs xPOP) |

**Tx total: 35 sketches**

## Archived (2026-09-28)

Moved (not deleted) to a maintainer archive:

| Family | Reason |
|--------|--------|
| NFToken* | Prefer URIToken; NonFungibleTokens* not enabled on Xahau |
| MPToken* | Amendment absent on mainnet+testnet |
| DID* / Credential* / PermissionedDomain* | Amendments absent on both nets |
| AMM* | Not live Xahau types |
| Vault* / Loan* / LoanBroker* (`Lending/`) | XRPL-port EncodeOnly |
| XChain* (`SideChains/`) | Intentional gap / not on public nets |
| Batch, DelegateSet | XRPL-port EncodeOnly |

Library `src` EncodeOnly fill/sign helpers were **also removed** (catalog codes only; bak a maintainer archive). See [ENCODE_ONLY_STUBS.md](../../docs/ENCODE_ONLY_STUBS.md).

## Notes

- Pseudo-transactions (`GenesisMint`, `EmitFailure`, `UNLReport`) have no examples.
- `LedgerStateFix` is catalog-only.
- Native amount enum remains `AmountKind::XrpDrops` (historical name); comments/docs prefer **XAH**.

## Shared helper

`src/xahau/sketch/example_runtime.h` - WiFi, network preset, sequence/fee, submit log, TLS notes.

## Security

Never commit real seeds or WiFi passwords. Prefer Testnet faucets.
See [docs/SECURITY.md](../../docs/SECURITY.md).
