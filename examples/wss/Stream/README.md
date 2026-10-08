# WSS - Stream (subscribe)

Push subscriptions over a persistent WebSocket session (stream monitor pattern

| Sketch | API | Notes |
|--------|-----|--------|
| [`LedgerStream`](./LedgerStream/LedgerStream.ino) | `subscribeLedger` + stream handler | Waits for N `ledgerClosed` events, then unsubscribes |
| [`AccountStream`](./AccountStream/AccountStream.ino) | `subscribeAccounts` + stream handler | Account watch; idle accounts may receive 0 txs |
| [`WalletMonitor`](./WalletMonitor/WalletMonitor.ino) | `subscribeEx` ledger + accounts (+ proposed) | **Recommended product pattern** |

## Flow

```text
WiFi -> WsXahauTransport -> setStreamHandler -> connect
     -> XahauRpcClient::subscribe* / subscribeEx
     -> poll() until done
     -> unsubscribe* -> close
```

**Do not** call `request()` / `rawRequest` / `subscribe*` from inside the stream handler (non-reentrant).

Prefer `ledger` + `accounts` on public nets (often Clio). Avoid global `transactions` / `validations` on ESP32.

## Related

- Request/response WSS: [`../Server/`](../Server/), [`../Account/`](../Account/)
- API: `docs/API.md` (subscribe + stream parsers)
- Security: keep TLS policy for Mainnet (`docs/SECURITY.md`)
