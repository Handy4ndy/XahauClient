# Encode-only catalog stubs — REMOVED from library source

On **2026-09-28**, EncodeOnly / XRPL-port **encode and sign helpers** for
non-Xahau families were **cut from library `src`** (not merely examples).

Archived source and former example trees are kept in the maintainer’s private
backup set (not part of the published library package). Do **not** restore those
families into live `examples/` or `src/` unless the matching amendment appears
enabled on both public nets (`feature` RPC) **and** maintainers ask.

Prefer **Testnet** (`XAHAU_NETWORK = 1`). Index `2+` also maps to Testnet
(`networkFromIndex`; no public Devnet preset).

Full live sketch tables: [EXAMPLES.md](EXAMPLES.md) · folder map: [examples/tx/README.md](../examples/tx/README.md)

## Prefer live alternatives

| Removed family (src + examples) | Prefer instead |
|---------------------------------|----------------|
| NFToken* | URITokenMint / Burn / CreateSellOffer / Buy / CancelSellOffer |
| Import | **Kept** — Xahau-native type **97** encode; live submit needs xPOP |
| XChain* | Out of scope (intentional open) |

## Families removed from library SOURCE (2026-09-28)

| Family | Former location | What remains |
|--------|-----------------|--------------|
| NFToken* | `phase2_extra` fill/sign | `TxType` catalog codes only (`TxMaturity::Catalog`) |
| AMM* | `phase4_extra` | Catalog codes only |
| Vault* / Loan* / LoanBroker* | `phase4_extra` / `loan.*` | Catalog codes only; `loan.*` deleted |
| DID* / Credential* / MPToken* / PermissionedDomain* | `phase4_extra` + sign helpers | Catalog codes only |
| Batch / DelegateSet | `phase4_extra` | Catalog codes only |
| XChain* (incl. attest) | `phase4_extra` / `xchain_attest.*` | Catalog codes only; `xchain_attest.*` deleted |
| RPC Vault/Loan ID helpers / `amm_info` builder | `rpc.*` / `client.*` | Generic `rpcParseObjectIds` kept; Clio NFT builders kept |
| Loan CounterpartySignature | `multi_sign.*` | Removed |

## Still in live library (Xahau-native / classic)

Payment path, AccountSet, TrustSet, Offer*, Escrow*, Check*, PayChan*, Tickets, RegularKey,
SignerList, DepositPreauth, AccountDelete, Clawback, Oracle*, URIToken*, Remit*, ClaimReward,
CronSet, SetRemarks, Invoke, SetHook, Import (encode type 97), plus network/RPC/WSS core.

`phase4_extra` now exposes **OracleSet only**.

## Feature check evidence (2026-09-28)

Public HTTPS `feature` on `https://xahau-test.net` (21338) and `https://xahau.network` (21337):
DID / Credentials / MPTokensV1 / PermissionedDomains / XChainBridge **absent**;
NonFungibleTokens* **enabled=false**; URIToken / Hooks / Import / Remit / Cron / Remarks /
Clawback / PriceOracle **present** (enabled on testnet; see NETWORKS for mainnet nuance).

Intentional opens (Import live / MCU SetHook serial / XChain / P1 absent amendments):
[REMAINING_GAPS.md](REMAINING_GAPS.md).
