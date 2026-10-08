// phase4_extra.cpp -- OracleSet only (cut EncodeOnly XRPL-port families 2026-09-28)
#include "phase4_extra.h"

#include "../codec/st_helpers.h"

#include <string.h>

namespace xahau {

static Status beginTx(TxFieldBag& bag, TxType tt, const TxCommon& c, const uint8_t* acc,
                      const uint8_t* pub) {
  if (!acc || !pub) return Status::InvalidArgument;
  bag.clear();
  return bagSetCommon(bag, tt, c, acc, pub, nullptr, 0, false);
}

static Status putBlobStr(TxFieldBag& bag, uint32_t code, const char* s) {
  if (!s || !s[0]) return Status::Ok;
  return bag.setBlob(code, reinterpret_cast<const uint8_t*>(s),
                     static_cast<uint16_t>(strlen(s)));
}

Status fillOracleSet(const OracleSetParams& p, const uint8_t* acc, const uint8_t* pub,
                     TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::OracleSet, p.common, acc, pub);
  if (st != Status::Ok) return st;
  // LastUpdateTime is UNIX epoch (XLS-47); must be within +/-300s of ledger close.
  st = bag.setUInt32(fc::LastUpdateTime, normalizeOracleUpdateTime(p.lastUpdateTime));
  if (st != Status::Ok) return st;
  st = bag.setUInt32(fc::OracleDocumentID, p.oracleDocumentID);
  if (st != Status::Ok) return st;
  if (p.provider[0]) {
    st = bag.setBlob(fc::Provider, reinterpret_cast<const uint8_t*>(p.provider),
                     static_cast<uint16_t>(strlen(p.provider)));
    if (st != Status::Ok) return st;
  }
  if (p.assetClass[0]) {
    st = bag.setBlob(fc::AssetClass, reinterpret_cast<const uint8_t*>(p.assetClass),
                     static_cast<uint16_t>(strlen(p.assetClass)));
    if (st != Status::Ok) return st;
  }
  st = putBlobStr(bag, fc::URI, p.uri);
  if (st != Status::Ok) return st;
  uint8_t series[640];
  size_t slen = 0;
  if (p.pairCount > 0) {
    if (p.pairCount > XAHAU_ORACLE_MAX_PAIRS) return Status::InvalidField;
    OraclePricePairWire wires[XAHAU_ORACLE_MAX_PAIRS];
    for (uint8_t i = 0; i < p.pairCount; ++i) {
      wires[i].baseAsset = p.pairs[i].baseAsset;
      wires[i].quoteAsset = p.pairs[i].quoteAsset;
      wires[i].assetPrice = p.pairs[i].assetPrice;
      wires[i].scale = p.pairs[i].scale;
      wires[i].hasPrice = p.pairs[i].hasPrice;
    }
    st = buildOraclePriceDataSeriesN(wires, p.pairCount, series, sizeof(series), slen);
  } else {
    st = buildOraclePriceDataSeries(p.baseAsset, p.quoteAsset, p.assetPrice, p.scale, p.hasPrice,
                                    series, sizeof(series), slen);
  }
  if (st != Status::Ok) return st;
  return bag.setEmbedded(fc::PriceDataSeries, series, static_cast<uint16_t>(slen));
}


}  // namespace xahau