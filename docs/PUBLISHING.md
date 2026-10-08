# Publishing checklist

Use this list before tagging a release or pushing to GitHub / Arduino Library Manager.

## 0. Pre-flight secrets

- [ ] Examples: `YOUR_WIFI_*`; live sketches `sEdPLACEHOLDER_*`; destinations/issuers `rYOUR_*_ADDRESS`
- [ ] Host tests: placeholders in the committed tree; real fixtures only in gitignored `test/native/test_seeds.local.h`
- [ ] No `secrets.h` / `secrets.json` / `test_seeds.local.h` staged
- [ ] No absolute machine paths in published metadata

## 1. Sanitise

- [ ] No real Wi-Fi SSIDs/passwords in any `.ino` or docs (only `YOUR_WIFI_*`)
- [ ] No real mainnet seeds or private keys in the committed tree
- [ ] No absolute local machine paths in committed metadata
- [ ] No `__pycache__`, `.pio`, or personal notes committed (see `.gitignore` / `library.json` export.exclude)

## 2. Version sync

Update **all** of:

- [ ] `library.properties` -> `version=`
- [ ] `library.json` -> `"version"`
- [ ] `src/XahauClient.h` -> `XAHAU_CLIENT_VERSION*`
- [ ] Root `README.md` version line
- [ ] Git tag `vX.Y.Z` matches version files (history via git tags / GitHub releases; no in-tree CHANGELOG)

## 3. Tests

```bash
# Windows example
set PATH=C:\msys64\mingw64\bin;%PATH%
pio test -e native
```

- [ ] With `test_seeds.local.h`: full suite green (123/123)
- [ ] Without local fixtures: encode-only green, seed tests skip (61 pass / 62 skip)
- [ ] Spot-check one ESP32 sketch compiles with `XAHAU_FEATURE_NET=1` and WebSockets

## 4. Docs

- [ ] README install URLs point at https://github.com/Handy4ndy/XahauClient
- [ ] EXAMPLES.md counts match the tree
- [ ] SECURITY.md / NETWORKS.md accurate
- [ ] LICENSE present; vendor NOTICE present

## 5. GitHub / registry

- [ ] `repository.url` in `library.json` is `https://github.com/Handy4ndy/XahauClient.git`
- [ ] `library.properties` `url=` is `https://github.com/Handy4ndy/XahauClient`
- [ ] Tag `vX.Y.Z` matches version files
- [ ] Optional: Arduino Library Manager submission after public tag

## 6. Do not publish

- Private product keys, faucet accounts with residual value, confidential internal notes
- `lib_extra_dirs` pointing at a personal absolute path (use relative or omit in published `platformio.ini`)
