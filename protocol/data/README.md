# Protocol data

Frozen Xahau field definitions used to generate C headers for the codec.

| File | Role |
|------|------|
| `field_registry.json` | Authoritative field table (~400 entries) checked into this repo |

Regenerate headers:

```bash
python protocol/scripts/generate_fields.py
# or from an external registry export:
# python protocol/scripts/generate_fields.py /path/to/field_registry.json
```

Outputs:

- `protocol/data/field_registry.json` (normalized freeze)
- `src/xahau/generated/protocol_fields.h`

Do not commit machine-specific absolute paths in the JSON `source` metadata.
