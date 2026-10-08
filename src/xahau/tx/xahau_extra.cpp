// xahau_extra.cpp - Xahau-native transaction builders
#include "xahau_extra.h"

#include "phase4_extra.h"

#include "../codec/binary_write.h"
#include "../codec/st_helpers.h"
#include "../core/hex.h"
#include "../crypto/base58.h"
#include "../sign/sign.h"

#include <string.h>

namespace xahau {

namespace {

Status beginTx(TxFieldBag& bag, TxType tt, const TxCommon& c, const uint8_t* acc,
               const uint8_t* pub) {
  return bagSetCommon(bag, tt, c, acc, pub, nullptr, 0, false);
}

Status putAddr(TxFieldBag& bag, uint32_t code, const char* classic) {
  if (!classic || !classic[0]) return Status::MissingField;
  uint8_t id[20];
  Status st = decodeClassicAddress(classic, id);
  if (st != Status::Ok) return st;
  return bag.setAccountID(code, id);
}

Status putAmount(TxFieldBag& bag, uint32_t code, const Amount& a) {
  if (a.kind == AmountKind::XrpDrops) {
    return bag.setAmountXrp(code, a.drops);
  }
  if (a.kind == AmountKind::Issued) {
    uint8_t iss[20];
    Status st = decodeClassicAddress(a.issued.issuer, iss);
    if (st != Status::Ok) return st;
    return bag.setAmountIssued(code, a.issued.value, a.issued.currency, iss);
  }
  return Status::InvalidField;
}

/** Append one AmountEntry { Amount } (no array end). */
Status appendRemitAmountEntry(const Amount& a, uint8_t* body, size_t& pos, size_t cap) {
  Status st = writeByte(body, pos, cap, 0xE0);  // STObject type14 extended
  if (st != Status::Ok) return st;
  st = writeByte(body, pos, cap, 0x5B);  // AmountEntry nth 91
  if (st != Status::Ok) return st;
  st = writeByte(body, pos, cap, 0x61);  // Amount type6 nth1
  if (st != Status::Ok) return st;
  if (a.kind == AmountKind::XrpDrops) {
    st = writeXrpAmount(body, pos, cap, a.drops);
    if (st != Status::Ok) return st;
  } else if (a.kind == AmountKind::Issued) {
    uint8_t iss[20];
    st = decodeClassicAddress(a.issued.issuer, iss);
    if (st != Status::Ok) return st;
    st = writeIssuedAmount(body, pos, cap, a.issued.value, a.issued.currency, iss);
    if (st != Status::Ok) return st;
  } else {
    return Status::InvalidField;
  }
  return writeByte(body, pos, cap, 0xE1);
}

/** Remit.Amounts body: N AmountEntry objects then array end. */
Status buildRemitAmounts(const Amount* amts, uint8_t count, uint8_t* body, size_t cap,
                         size_t& outLen) {
  if (!amts || count == 0) return Status::MissingField;
  size_t pos = 0;
  for (uint8_t i = 0; i < count; ++i) {
    Status st = appendRemitAmountEntry(amts[i], body, pos, cap);
    if (st != Status::Ok) return st;
  }
  Status st = writeByte(body, pos, cap, 0xF1);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

Status buildRemitAmountsOne(const Amount& a, uint8_t* body, size_t cap, size_t& outLen) {
  return buildRemitAmounts(&a, 1, body, cap, outLen);
}

/** MintURIToken STObject body (inner fields + E1; field header via setEmbedded). */
Status buildMintURIToken(const char* uri, bool hasDigest, const char* digestHex, uint32_t flags,
                         uint8_t* body, size_t cap, size_t& outLen) {
  if (!uri || !uri[0]) return Status::MissingField;
  size_t pos = 0;
  Status st;
  if (flags != 0) {
    st = writeByte(body, pos, cap, 0x22);
    if (st != Status::Ok) return st;
    st = writeUInt32BE(body, pos, cap, flags);
    if (st != Status::Ok) return st;
  }
  if (hasDigest && digestHex && digestHex[0]) {
    uint8_t dig[32];
    size_t n = 0;
    st = hexToBytes(digestHex, dig, 32, n);
    if (st != Status::Ok || n != 32) return Status::InvalidField;
    st = writeByte(body, pos, cap, 0x50);
    if (st != Status::Ok) return st;
    st = writeByte(body, pos, cap, 0x15);  // Digest Hash256 nth 21
    if (st != Status::Ok) return st;
    st = writeBytes(body, pos, cap, dig, 32);
    if (st != Status::Ok) return st;
  }
  size_t ulen = strlen(uri);
  if (ulen >= 193) return Status::InvalidField;
  st = writeByte(body, pos, cap, 0x75);  // URI Blob 7,5
  if (st != Status::Ok) return st;
  st = writeByte(body, pos, cap, static_cast<uint8_t>(ulen));
  if (st != Status::Ok) return st;
  st = writeBytes(body, pos, cap, reinterpret_cast<const uint8_t*>(uri), ulen);
  if (st != Status::Ok) return st;
  st = writeByte(body, pos, cap, 0xE1);
  if (st != Status::Ok) return st;
  outLen = pos;
  return Status::Ok;
}

#define SIGN_HELPER(Name, ParamsT, fillFn)                                                       \
  Status buildAndSign##Name(const KeyPair& keys, const ParamsT& p, char* outHex,                 \
                            size_t outHexCap) {                                                  \
    TxFieldBag& bag = scratchSignBag();                                                          \
    Status st = fillFn(p, keys.accountId, keys.publicKey, bag);                                  \
    if (st != Status::Ok) return st;                                                             \
    return signTx(bag, keys, outHex, outHexCap);                                                 \
  }

}  // namespace

Status fillInvoke(const InvokeParams& p, const uint8_t* acc, const uint8_t* pub, TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::Invoke, p.common, acc, pub);
  if (st != Status::Ok) return st;
  if (p.destination[0]) {
    st = putAddr(bag, fc::Destination, p.destination);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillClaimReward(const ClaimRewardParams& p, const uint8_t* acc, const uint8_t* pub,
                       TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::ClaimReward, p.common, acc, pub);
  if (st != Status::Ok) return st;
  // Opt-out: Flags tfOptOut=1, no Issuer / ClaimCurrency.
  if (p.optOut) {
    return bag.setUInt32(fc::Flags, p.common.flags | 1u);  // tfOptOut

  }
  if (p.issuer[0]) {
    st = putAddr(bag, fc::Issuer, p.issuer);
    if (st != Status::Ok) return st;
  }
  // IOURewardClaim: ClaimCurrency Issue {currency, issuer}
  if (p.claimCurrency[0]) {
    const char* curIss = p.claimCurrencyIssuer[0] ? p.claimCurrencyIssuer : p.issuer;
    if (!curIss || !curIss[0]) return Status::MissingField;
    uint8_t iss[20];
    st = decodeClassicAddress(curIss, iss);
    if (st != Status::Ok) return st;
    st = bag.setIssueIou(fc::ClaimCurrency, p.claimCurrency, iss);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillImport(const ImportParams& p, const uint8_t* acc, const uint8_t* pub, TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::Import, p.common, acc, pub);
  if (st != Status::Ok) return st;
  if (!p.blob || p.blobLen == 0) return Status::MissingField;
  st = bag.setBlob(fc::Blob, p.blob, p.blobLen);
  if (st != Status::Ok) return st;
  if (p.issuer[0]) {
    st = putAddr(bag, fc::Issuer, p.issuer);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillRemit(const RemitParams& p, const uint8_t* acc, const uint8_t* pub, TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::Remit, p.common, acc, pub);
  if (st != Status::Ok) return st;
  st = putAddr(bag, fc::Destination, p.destination);
  if (st != Status::Ok) return st;

  const Amount* amts = nullptr;
  uint8_t acount = 0;
  if (p.amountCount > 0) {
    amts = p.amounts;
    acount = p.amountCount > 4 ? 4 : p.amountCount;
  } else if (p.amount.kind != AmountKind::None) {
    amts = &p.amount;
    acount = 1;
  }
  if (acount > 0) {
    uint8_t body[384];
    size_t blen = 0;
    st = buildRemitAmounts(amts, acount, body, sizeof(body), blen);
    if (st != Status::Ok) return st;
    st = bag.setEmbedded(fc::Amounts, body, static_cast<uint16_t>(blen));
    if (st != Status::Ok) return st;
  }

  if (p.uriTokenIdCount > 0) {
    uint8_t n = p.uriTokenIdCount > 4 ? 4 : p.uriTokenIdCount;
    uint8_t ids[4 * 32];
    for (uint8_t i = 0; i < n; ++i) memcpy(ids + i * 32, p.uriTokenIds[i], 32);
    // Vector256 is VL-encoded like Blob
    st = bag.setBlob(fc::URITokenIDs, ids, static_cast<uint16_t>(n * 32));
    if (st != Status::Ok) return st;
  }

  if (p.mintUri[0]) {
    uint8_t body[320];
    size_t blen = 0;
    st = buildMintURIToken(p.mintUri, p.mintHasDigest, p.mintDigestHex, p.mintFlags, body,
                           sizeof(body), blen);
    if (st != Status::Ok) return st;
    st = bag.setEmbedded(fc::MintURIToken, body, static_cast<uint16_t>(blen));
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillCronSet(const CronSetParams& p, const uint8_t* acc, const uint8_t* pub,
                   TxFieldBag& bag) {
  TxCommon common = p.common;
  if (p.unset) {
    // tfCronUnset=1; must omit StartTime / RepeatCount / DelaySeconds.
    common.flags = (common.flags & ~0x00000001u) | 0x00000001u;
  }
  Status st = beginTx(bag, TxType::CronSet, common, acc, pub);
  if (st != Status::Ok) return st;
  if (p.unset) return Status::Ok;
  // Always emit StartTime on create: 0 means ASAP (omit was temMALFORMED on testnet).
  st = bag.setUInt32(fc::StartTime, p.startTime);
  if (st != Status::Ok) return st;
  st = bag.setUInt32(fc::RepeatCount, p.repeatCount);
  if (st != Status::Ok) return st;
  return bag.setUInt32(fc::DelaySeconds, p.delaySeconds);
}

Status fillSetRemarks(const SetRemarksParams& p, const uint8_t* acc, const uint8_t* pub,
                      TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::SetRemarks, p.common, acc, pub);
  if (st != Status::Ok) return st;
  if (!p.remarkName[0]) return Status::MissingField;
  // Reject all-zero ObjectID (required Hash256 of target ledger object).
  bool oidZero = true;
  for (int i = 0; i < 32; ++i) {
    if (p.objectId[i] != 0) { oidZero = false; break; }
  }
  if (oidZero) return Status::MissingField;
  st = bag.setHash256(fc::ObjectID, p.objectId);
  if (st != Status::Ok) return st;

  // Minimal Remarks array: one Remark object with Name + Value blobs.
  // Object type 14 nth 97 -> header 0xE0, 0x61
  uint8_t body[256];
  size_t pos = 0;
  auto wb = [&](uint8_t b) -> Status {
    if (pos >= sizeof(body)) return Status::BufferTooSmall;
    body[pos++] = b;
    return Status::Ok;
  };
  st = wb(0xE0);
  if (st != Status::Ok) return st;
  st = wb(0x61);
  if (st != Status::Ok) return st;
  size_t nlen = strlen(p.remarkName);
  size_t vlen = strlen(p.remarkValue);
  // Canonical STObject order: Flags (2,2) then RemarkValue (7,98) then RemarkName (7,99).
  if (p.immutable) {
    st = wb(0x22);
    if (st != Status::Ok) return st;
    st = wb(0);
    if (st != Status::Ok) return st;
    st = wb(0);
    if (st != Status::Ok) return st;
    st = wb(0);
    if (st != Status::Ok) return st;
    st = wb(1);
    if (st != Status::Ok) return st;
  }
  if (vlen > 0) {
    // RemarkValue Blob 7,98 -> 0x70, 0x62
    st = wb(0x70);
    if (st != Status::Ok) return st;
    st = wb(0x62);
    if (st != Status::Ok) return st;
    if (vlen >= 193) return Status::InvalidField;
    st = wb(static_cast<uint8_t>(vlen));
    if (st != Status::Ok) return st;
    if (pos + vlen > sizeof(body)) return Status::BufferTooSmall;
    memcpy(body + pos, p.remarkValue, vlen);
    pos += vlen;
  }
  // RemarkName Blob 7,99 -> 0x70, 0x63
  st = wb(0x70);
  if (st != Status::Ok) return st;
  st = wb(0x63);
  if (st != Status::Ok) return st;
  if (nlen >= 193) return Status::InvalidField;
  st = wb(static_cast<uint8_t>(nlen));
  if (st != Status::Ok) return st;
  if (pos + nlen > sizeof(body)) return Status::BufferTooSmall;
  memcpy(body + pos, p.remarkName, nlen);
  pos += nlen;
  st = wb(0xE1);
  if (st != Status::Ok) return st;
  st = wb(0xF1);
  if (st != Status::Ok) return st;
  return bag.setEmbedded(fc::Remarks, body, static_cast<uint16_t>(pos));
}
Status fillURITokenMint(const URITokenMintParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::URITokenMint, p.common, acc, pub);
  if (st != Status::Ok) return st;
  if (!p.uri[0]) return Status::MissingField;
  st = bag.setBlob(fc::URI, reinterpret_cast<const uint8_t*>(p.uri),
                   static_cast<uint16_t>(strlen(p.uri)));
  if (st != Status::Ok) return st;
  if (p.hasDigest && p.digestHex[0]) {
    uint8_t dig[32];
    size_t n = 0;
    st = hexToBytes(p.digestHex, dig, 32, n);
    if (st != Status::Ok || n != 32) return Status::InvalidField;
    st = bag.setHash256(fc::Digest, dig);
    if (st != Status::Ok) return st;
  }
  if (p.amount.kind != AmountKind::None) {
    st = putAmount(bag, fc::Amount, p.amount);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillURITokenBurn(const URITokenBurnParams& p, const uint8_t* acc, const uint8_t* pub,
                        TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::URITokenBurn, p.common, acc, pub);
  if (st != Status::Ok) return st;
  return bag.setHash256(fc::URITokenID, p.uriTokenId);
}

Status fillURITokenBuy(const URITokenBuyParams& p, const uint8_t* acc, const uint8_t* pub,
                       TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::URITokenBuy, p.common, acc, pub);
  if (st != Status::Ok) return st;
  st = bag.setHash256(fc::URITokenID, p.uriTokenId);
  if (st != Status::Ok) return st;
  if (p.amount.kind == AmountKind::None) return Status::MissingField;
  return putAmount(bag, fc::Amount, p.amount);
}

Status fillURITokenCreateSellOffer(const URITokenCreateSellOfferParams& p, const uint8_t* acc,
                                   const uint8_t* pub, TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::URITokenCreateSellOffer, p.common, acc, pub);
  if (st != Status::Ok) return st;
  st = bag.setHash256(fc::URITokenID, p.uriTokenId);
  if (st != Status::Ok) return st;
  if (p.amount.kind == AmountKind::None) return Status::MissingField;
  st = putAmount(bag, fc::Amount, p.amount);
  if (st != Status::Ok) return st;
  if (p.destination[0]) {
    st = putAddr(bag, fc::Destination, p.destination);
    if (st != Status::Ok) return st;
  }
  return Status::Ok;
}

Status fillURITokenCancelSellOffer(const URITokenCancelSellOfferParams& p, const uint8_t* acc,
                                   const uint8_t* pub, TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::URITokenCancelSellOffer, p.common, acc, pub);
  if (st != Status::Ok) return st;
  return bag.setHash256(fc::URITokenID, p.uriTokenId);
}

#ifndef XAHAU_SETHOOK_BODY_CAP
// Tier A SetHook Hooks STArray workspace: CreateCode <= 8 KiB + Hook object overhead.
#define XAHAU_SETHOOK_BODY_CAP 8448
#endif

Status fillSetHook(const SetHookParams& p, const uint8_t* acc, const uint8_t* pub,
                   TxFieldBag& bag) {
  Status st = beginTx(bag, TxType::SetHook, p.common, acc, pub);
  if (st != Status::Ok) return st;
  if (!p.createCode || p.createCodeLen == 0) return Status::MissingField;

  // Hooks STArray body: one Hook STObject (type14 nth14 => single-byte 0xEE).
  // Inner fields MUST be canonical order (type then nth):
  //   HookApiVersion (1,20), Flags (2,2), HookOn (5,20), HookNamespace (5,32),
  //   CreateCode (7,11) then object end 0xE1, array end 0xF1.
  // Tier A: CreateCode <= 8192. VL uses 2-byte form for len in (193..12480].
  // Build off Arduino loopTask stack (static BSS); host uses stack for re-entrancy.
#if defined(ARDUINO) || defined(XAHAU_PLATFORM_ARDUINO)
  static uint8_t body[XAHAU_SETHOOK_BODY_CAP];
#else
  uint8_t body[XAHAU_SETHOOK_BODY_CAP];
#endif
  size_t pos = 0;
  auto ensure = [&](size_t n) -> bool { return pos + n <= sizeof(body); };


  if (!ensure(1)) return Status::BufferTooSmall;
  body[pos++] = 0xEE;  // Hook STObject header (14,14)

  // HookApiVersion UInt16 type 1 field 20 => 0x10, 0x14
  if (!ensure(4)) return Status::BufferTooSmall;
  body[pos++] = 0x10;
  body[pos++] = 0x14;
  body[pos++] = (uint8_t)(p.hookApiVersion >> 8);
  body[pos++] = (uint8_t)(p.hookApiVersion);

  // Flags UInt32 type 2 field 2 => 0x22 (omit when 0)
  if (p.hookFlags != 0) {
    if (!ensure(5)) return Status::BufferTooSmall;
    body[pos++] = 0x22;
    body[pos++] = (uint8_t)(p.hookFlags >> 24);
    body[pos++] = (uint8_t)(p.hookFlags >> 16);
    body[pos++] = (uint8_t)(p.hookFlags >> 8);
    body[pos++] = (uint8_t)(p.hookFlags);
  }

  // HookOn Hash256 type 5 field 20 => 0x50, 0x14 + 32
  if (!ensure(2 + 32)) return Status::BufferTooSmall;
  body[pos++] = 0x50;
  body[pos++] = 0x14;
  memcpy(body + pos, p.hookOn, 32);
  pos += 32;

  // HookNamespace Hash256 type 5 field 32 => 0x50, 0x20 + 32
  if (!ensure(2 + 32)) return Status::BufferTooSmall;
  body[pos++] = 0x50;
  body[pos++] = 0x20;
  memcpy(body + pos, p.hookNamespace, 32);
  pos += 32;

  // CreateCode Blob type 7 field 11 => single-byte 0x7B + VL + bytes
  // Tier A CreateCode <= 8192 uses 2-byte VL (len <= 12480); 3-byte not needed yet.
  if (!ensure(1)) return Status::BufferTooSmall;
  body[pos++] = 0x7B;
  if (p.createCodeLen <= 192) {
    if (!ensure(1 + p.createCodeLen)) return Status::BufferTooSmall;
    body[pos++] = (uint8_t)p.createCodeLen;
  } else if (p.createCodeLen <= 12480) {
    uint16_t L = static_cast<uint16_t>(p.createCodeLen - 193);
    if (!ensure(2 + p.createCodeLen)) return Status::BufferTooSmall;
    body[pos++] = (uint8_t)(193 + (L >> 8));
    body[pos++] = (uint8_t)(L & 0xFF);
  } else {
    return Status::InvalidField;
  }
  memcpy(body + pos, p.createCode, p.createCodeLen);
  pos += p.createCodeLen;

  if (!ensure(2)) return Status::BufferTooSmall;
  body[pos++] = 0xE1;  // object end
  body[pos++] = 0xF1;  // array end

  return bag.setEmbedded(fc::Hooks, body, static_cast<uint16_t>(pos));
}

SIGN_HELPER(Invoke, InvokeParams, fillInvoke)
SIGN_HELPER(ClaimReward, ClaimRewardParams, fillClaimReward)
SIGN_HELPER(Remit, RemitParams, fillRemit)
SIGN_HELPER(Import, ImportParams, fillImport)
SIGN_HELPER(CronSet, CronSetParams, fillCronSet)
SIGN_HELPER(SetRemarks, SetRemarksParams, fillSetRemarks)
SIGN_HELPER(URITokenMint, URITokenMintParams, fillURITokenMint)
SIGN_HELPER(URITokenBurn, URITokenBurnParams, fillURITokenBurn)
SIGN_HELPER(URITokenBuy, URITokenBuyParams, fillURITokenBuy)
SIGN_HELPER(URITokenCreateSellOffer, URITokenCreateSellOfferParams, fillURITokenCreateSellOffer)
SIGN_HELPER(URITokenCancelSellOffer, URITokenCancelSellOfferParams, fillURITokenCancelSellOffer)

Status buildAndSignOracleSet(const KeyPair& keys, const OracleSetParams& p, char* outHex,
                             size_t outHexCap) {
  TxFieldBag& bag = scratchSignBag();
  Status st = fillOracleSet(p, keys.accountId, keys.publicKey, bag);
  if (st != Status::Ok) return st;
  return signTx(bag, keys, outHex, outHexCap);
}


// ---------------------------------------------------------------------------
// Cron (92) emitted-tx - minimal field walker (not a general FieldReader)
// ---------------------------------------------------------------------------
namespace {

Status readVlLength(const uint8_t* blob, size_t len, size_t& i, size_t& outLen) {
  if (i >= len) return Status::InvalidField;
  uint8_t b = blob[i++];
  if (b <= 192) {
    outLen = b;
    return Status::Ok;
  }
  if (b <= 240) {
    if (i >= len) return Status::InvalidField;
    outLen = 193u + (static_cast<size_t>(b - 193) << 8) + blob[i++];
    return Status::Ok;
  }
  if (b <= 254) {
    if (i + 1 >= len) return Status::InvalidField;
    outLen = 12481u + (static_cast<size_t>(b - 241) << 16) +
             (static_cast<size_t>(blob[i]) << 8) + blob[i + 1];
    i += 2;
    return Status::Ok;
  }
  return Status::InvalidField;
}

Status readFieldHeader(const uint8_t* blob, size_t len, size_t& i, uint16_t& type,
                       uint16_t& field) {
  if (i >= len) return Status::InvalidField;
  uint8_t c = blob[i++];
  type = static_cast<uint16_t>(c >> 4);
  field = static_cast<uint16_t>(c & 0x0f);
  if (type == 0) {
    if (i >= len) return Status::InvalidField;
    type = blob[i++];
  }
  if (field == 0) {
    if (i >= len) return Status::InvalidField;
    field = blob[i++];
  }
  return Status::Ok;
}

}  // namespace

Status decodeEmittedCron(const uint8_t* blob, size_t len, CronEmittedView& out) {
  if (!blob || len == 0) return Status::InvalidArgument;
  out = CronEmittedView();

  size_t i = 0;
  bool sawType = false;
  uint16_t txType = 0;

  while (i < len) {
    uint16_t type = 0, field = 0;
    Status st = readFieldHeader(blob, len, i, type, field);
    if (st != Status::Ok) return st;

    if (type == 1) {  // UInt16
      if (i + 2 > len) return Status::InvalidField;
      uint16_t v = static_cast<uint16_t>((blob[i] << 8) | blob[i + 1]);
      i += 2;
      if (field == 2) {
        txType = v;
        sawType = true;
      }
    } else if (type == 2) {  // UInt32
      if (i + 4 > len) return Status::InvalidField;
      uint32_t v = (static_cast<uint32_t>(blob[i]) << 24) |
                   (static_cast<uint32_t>(blob[i + 1]) << 16) |
                   (static_cast<uint32_t>(blob[i + 2]) << 8) |
                   static_cast<uint32_t>(blob[i + 3]);
      i += 4;
      if (field == 4) out.sequence = v;
      else if (field == 6) out.ledgerSequence = v;
    } else if (type == 6) {  // Amount (Cron Fee is XRP-only)
      if (i >= len) return Status::InvalidField;
      if (blob[i] & 0x80u) {
        if (i + 48 > len) return Status::InvalidField;
        i += 48;
      } else {
        if (i + 8 > len) return Status::InvalidField;
        i += 8;
      }
    } else if (type == 7 || type == 8) {  // Blob / AccountID (VL)
      size_t vl = 0;
      st = readVlLength(blob, len, i, vl);
      if (st != Status::Ok) return st;
      if (i + vl > len) return Status::InvalidField;
      if (type == 8 && field == 2 && vl == 20) {
        st = encodeClassicAddress(blob + i, out.owner);
        if (st != Status::Ok) return st;
      }
      i += vl;
    } else if (type == 4) {  // Hash128
      if (i + 16 > len) return Status::InvalidField;
      i += 16;
    } else if (type == 5) {  // Hash256
      if (i + 32 > len) return Status::InvalidField;
      i += 32;
    } else if (type == 3) {  // UInt64
      if (i + 8 > len) return Status::InvalidField;
      i += 8;
    } else {
      // Unknown STI type - cannot skip without a full FieldReader.
      return Status::NotImplemented;
    }
  }

  if (!sawType || txType != static_cast<uint16_t>(TxType::Cron)) {
    return Status::InvalidField;
  }
  return Status::Ok;
}

}  // namespace xahau
