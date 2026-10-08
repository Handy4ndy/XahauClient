# XahauClient RPC examples (offline / mock)

**No Wi-Fi.** Pure JSON-RPC **builders**, **parsers**, and `XahauRpcClient` over **`MockXahauTransport`**.

Live ledger I/O over WebSocket: **[`../wss/`](../wss/)**.

| Sketch | Purpose |
|--------|---------|
| [`BuildCatalog`](./BuildCatalog/) | **All** public `rpcBuild*` (account / server / submit / subscribe) using **A+B** seeds |
| [`ParseCatalog`](./ParseCatalog/) | **All** major `rpcParse*` + stream event parsers (canned JSON) |
| [`MockClientFlow`](./MockClientFlow/) | Mock client: **A→B** Payment, submit, `waitForOutcome`, subscribe |

Legacy thin sketches (`BuildAccountInfo`, …) may remain for IDE history; prefer the catalog sketches above for coverage.

## Host tests

From the library root:

```bash
pio test -e native -f test_rpc
```

## Success markers

Serial (when run on-device with a mock sketch) should end with **`Done.`** and include **`: Ok`** lines for catalog steps.
