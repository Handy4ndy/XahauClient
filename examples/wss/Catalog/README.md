# WSS Catalog — live RPC method batteries

Online (Wi-Fi + public WSS) exercises of **wallet-priority** `rpcBuild*` commands.

| Sketch | Methods |
|--------|---------|
| [`AccountRpcLive`](./AccountRpcLive/) | account_channels, currencies, nfts, offers, gateway_balances, noripple_check, lines, objects, tx, info |
| [`LedgerServerRpcLive`](./LedgerServerRpcLive/) | ping, fee, feature, random, server_*, ledger*, ledger_entry (vault_info builder removed) |
| [`PathNftRpcLive`](./PathNftRpcLive/) | deposit_authorized, book_offers; nft_* (Clio) often server-error on Xahau (prefer URIToken); channel_verify; AMM info builder removed |

**Success:** transport round-trip (`build` + `rawRequest` → `Ok`). Server-side `error` (empty book, unknown NFT) is printed but **does not** fail the sketch.

Offline builders: `examples/rpc/BuildCatalog`.
