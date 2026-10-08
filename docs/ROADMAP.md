# Roadmap

Historical phase checklist. For **current** 0.3.1 ship surface and intentional opens, see [LIBRARY_ROADMAP.md](LIBRARY_ROADMAP.md) and [REMAINING_GAPS.md](REMAINING_GAPS.md). Several Phase 4 nested fills (DID / Credential / MPT / Domain / XChain / Vault / Loan / Batch) were later removed from library `src` (catalog codes only) — see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md).

## Phase 0–3

- [x] Scaffold, codec, dual-curve keys, core tx fills, RPC + mock/WS transport
- Host tests through Phase 3: green

## Phase 4 — Protocol automation + nested types (**COMPLETE**)

### Protocol automation

- [x] `protocol/scripts/generate_fields.py` freezes `field_registry.json` (400 fields)
- [x] Generated `src/xahau/generated/protocol_fields.h` + `findField()`
- [x] Codec extensions: Issue (type 24), Hash192, larger arena, nested ST helpers

### Nested / advanced fills (HISTORICAL checklist)

> **Note (0.3.1):** these Phase 4 fills were completed historically, then **removed from library `src` on 2026-09-28** except **OracleSet** (still shipped). Survivors are catalog `TxType` codes only — see [ENCODE_ONLY_STUBS.md](ENCODE_ONLY_STUBS.md). Do not treat the checkmarks below as current public APIs.

- [x] DIDSet / DIDDelete *(removed from src)*
- [x] CredentialCreate / Accept / Delete *(removed from src)*
- [x] AMMCreate / Deposit / Withdraw / Vote / Bid / Delete / Clawback *(removed from src)*
- [x] OracleSet (PriceDataSeries) — **still shipped**
- [x] MPTokenIssuanceCreate / Destroy / Set / Authorize *(removed from src)*
- [x] PermissionedDomainSet / Delete *(removed from src)*
- [x] DelegateSet *(removed from src)*
- [x] Batch (empty RawTransactions) *(removed from src)*
- [x] VaultCreate / Set / Delete / Deposit / Withdraw / Clawback *(removed from src)*
- [x] XChainCreateBridge / ModifyBridge / CreateClaimID / Commit / Claim / AccountCreateCommit *(removed from src)*

### Loans (HISTORICAL)

- [x] LoanBrokerSet / Delete / CoverDeposit / CoverWithdraw / CoverClawback
- [x] LoanSet / Delete / Manage / Pay
- [x] Network presets: `kNetworkTestnet`, `kNetworkMainnet` (no public Devnet; index 2+ maps to Testnet)
- [x] Full tx + RPC example sets

### Still optional / advanced (not in 0.3.1 ship surface)

- XChain attestation collection txs — multi-object proofs *(out of scope)*
- Full STNumber encoder (historical note; Vault helpers removed from `src`)

## Phase 5 — Product integration (out of library scope until requested)

- [ ] Downstream products consume XahauClient as a library
