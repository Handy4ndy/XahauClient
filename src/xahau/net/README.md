# net/ — optional network transports

## Mock (always available)

`Xahau/rpc/mock_transport.h` — scripted responses for host unit tests.

## ESP32 WebSocket

Compile with:

```
-DXAHAU_FEATURE_NET=1
```

Requires Arduino ESP32 + **WebSockets** library and an active Wi‑Fi STA connection.

```cpp
#include <XahauClient.h>
#include <xahau/net/ws_transport.h>

// Official public Xahau Testnet WSS (port 443). See docs/NETWORKS.md.
WsXahauTransport ws("xahau-test.net", 443, "/");
// Prefer: WsXahauTransport ws(kNetworkTestnet);
XahauRpcClient client(ws);

uint32_t seq, lls;
client.fetchSequenceAndLls(address, seq, lls);
```
