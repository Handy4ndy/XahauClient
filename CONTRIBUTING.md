# Contributing to XahauClient

Thank you for improving the library. This document covers the minimum expectations for pull requests.

## Development setup

1. Clone the repository.
2. Install [PlatformIO](https://platformio.org/) and a C++17 toolchain (on Windows, MSYS2 MinGW is common).
3. From the library root:

```bash
pio test -e native
```

All host tests must pass before a PR is ready.


## Host test seeds

Committed tests use **placeholders** only. For the full native suite:

1. Copy `test/native/test_seeds.example.h` to `test/native/test_seeds.local.h`.
2. Fill fixture seeds / addresses in the local file (gitignored - never commit it).
3. Run `pio test -e native`.

Without `test_seeds.local.h`, seed-dependent tests **skip** with a clear message; encode-only tests still pass (typically **61 pass / 62 skip**). With fixtures filled, expect **123/123**.

Examples ship `YOUR_WIFI_*`, `sEdPLACEHOLDER_*`, and `rYOUR_DESTINATION_ADDRESS` / `rYOUR_ISSUER_ADDRESS`. Put real Wi-Fi and seeds in a **gitignored** `secrets.h` (or equivalent) for local flashes.

## Project rules

1. **Portable core** - no `Serial`, Wi-Fi, or NVS inside `codec/`, `crypto/`, or `sign/`.
2. **One sign path** - transactions fill a `TxFieldBag`, then `signTx`. Avoid ad-hoc binary emitters.
3. **No secrets** - never commit seeds, passwords, or private keys. Use placeholders in examples.
4. **Host tests first** - add Unity tests under `test/native/` for codec, crypto, and RPC logic.

## Style

- Prefer clear C++17 suitable for embedded compilers.
- Match existing naming: `fillX` / `buildAndSignX`, `rpcBuildX`, `Status` returns.
- Keep examples self-contained with a CONFIG block at the top.

## Generated code

Protocol fields:

```bash
python protocol/scripts/generate_fields.py
```

Examples (optional regeneration):

```bash
python protocol/scripts/generate_examples.py
python protocol/scripts/reorganize_examples.py
python protocol/scripts/generate_rpc_examples.py
```

Commit generated outputs when they change intentionally.

## Pull requests

- Describe **what** changed and **why**.
- Link related Xahau docs or amendment names when adding transaction support.
- Note user-visible changes in the PR description (this package does not ship a CHANGELOG file).
- Keep PRs focused; large refactors should be discussed first.

## Security issues

Please report security-sensitive bugs privately to the maintainer listed in `library.properties` rather than opening a public issue with exploit details.

See [docs/SECURITY.md](docs/SECURITY.md).

## License

By contributing, you agree that your contributions are licensed under the MIT License (see [LICENSE](LICENSE)).
