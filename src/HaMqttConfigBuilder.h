#pragma once
/**
 * HaMqttConfigBuilder
 * -----------------------------------------------------------------
 * Ultra-small Home Assistant MQTT Discovery JSON Builder
 *   • Header-only, no STL containers, no ArduinoJson
 *   • Single preallocated string as buffer → minimal RAM usage
 *   • Nested objects (Default maxDepth = 6)
 *   • Integer, Float, Bool, String fields without temporary objects
 *   • Simple top-level lookup via getString(key, result)
 *   • **Device caching**: `beginDevice()` / `endDevice()` + `nextSensor()`
 *     → Device block is retained, sensor part is overwritten
 */

#include <Arduino.h>
#include <stdlib.h>   // dtostrf

class HaMqttConfigBuilder {
public:
  static constexpr uint8_t kStaticMaxDepth = 6;   ///< Compile-time depth limit

  explicit HaMqttConfigBuilder(size_t reserveBytes = 256, uint8_t maxDepth = 4)
      : maxDepth_(maxDepth > kStaticMaxDepth ? kStaticMaxDepth : maxDepth) {
    payload_.reserve(reserveBytes);
    payload_ = '{';
    depth_   = 0;
    memset(first_, 1, sizeof(first_));
  }

  /** Clear everything (including device block). */
  void clear() {
    payload_.remove(0);
    payload_ = '{';
    depth_   = 0;
    memset(first_, 1, sizeof(first_));
    deviceSet_ = false;
    deviceEndPos_ = 0;
  }

  /* ------------------ Append fields ------------------ */

  HaMqttConfigBuilder &add(const char *key, const char *val) {
    beginField(key);
    payload_ += '"';
    escapeAndAppend(val);
    payload_ += '"';
    return *this;
  }
  HaMqttConfigBuilder &add(const char *key, const String &val) { return add(key, val.c_str()); }

  HaMqttConfigBuilder &add(const char *key, long val) {
    beginField(key);
    payload_.concat(val);
    return *this;
  }

  HaMqttConfigBuilder &add(const char *key, float val, uint8_t decimals = 2) {
    beginField(key);
    char buf[32];
    dtostrf(val, 0, decimals, buf);
    payload_ += buf;
    return *this;
  }

  HaMqttConfigBuilder &add(const char *key, bool val) {
    beginField(key);
    payload_ += (val ? F("true") : F("false"));
    return *this;
  }

  /* -------------- Nested objects -------------- */

  HaMqttConfigBuilder &beginObject(const char *key) {
    if (depth_ >= maxDepth_) return *this;
    beginField(key);
    payload_ += '{';
    ++depth_;
    first_[depth_] = true;
    return *this;
  }

  HaMqttConfigBuilder &endObject() {
    if (depth_ == 0) return *this;
    payload_ += '}';
    --depth_;
    return *this;
  }

  /* -------------- Device block shortcuts -------------- */

  HaMqttConfigBuilder &beginDevice() {
    // Device can only be created once per clear
    if (deviceSet_) return *this;               // already set
    beginObject("device");
    return *this;
  }

  HaMqttConfigBuilder &endDevice() {
    if (deviceSet_) return *this;               // already finished
    endObject();                                // closes "device"
    deviceSet_   = true;
    deviceEndPos_ = payload_.length();          // remember position after '}'
    first_[0]    = false;                       // root already has entry
    return *this;
  }

  /**
   * Removes the last appended sensor fields, but keeps the device block.
   * Afterwards, new fields can be appended via add().
   */
  void nextSensor() {
    if (!deviceSet_) return;                    // no device → nothing to do
    // remove everything after deviceEndPos_ (including previous sensor fields)
    payload_.remove(deviceEndPos_);
    depth_ = 0;                                // back to root level
    // root already has an entry (device), so not first
    first_[0] = false;
  }

  /* ----------------- Generate JSON ----------------- */

  String generate() const {
    String out = payload_;
    for (int8_t d = depth_; d >= 0; --d) out += '}';
    return out;
  }

  /* ----------- Simple top-level lookup ----------- */

  bool getString(const char *key, String &result) const {
    String json = generate();
    String needle = '"' + String(key) + '"';

    int pos = json.indexOf(needle);
    if (pos < 0) return false;

    pos = json.indexOf(':', pos + needle.length());
    if (pos < 0) return false;
    ++pos;

    while (pos < (int)json.length() && isspace(json[(size_t)pos])) ++pos;
    if (pos >= (int)json.length() || json[(size_t)pos] != '"') return false;
    ++pos;

    int end = pos;
    while (end < (int)json.length() && (json[(size_t)end] != '"' || json[(size_t)end - 1] == '\\')) ++end;
    if (end >= (int)json.length()) return false;

    result = json.substring(pos, end);
    return true;
  }

private:
  String        payload_;
  uint8_t       depth_  = 0;
  const uint8_t maxDepth_;
  bool          first_[kStaticMaxDepth + 1];

  bool   deviceSet_   = false;  ///< Device block has been finished
  size_t deviceEndPos_ = 0;     ///< Index after '}' of device object

  /* ----------- Internal helper for key entry ----------- */
  inline void beginField(const char *key) {
    if (!first_[depth_]) payload_ += ',';
    else                 first_[depth_] = false;

    payload_ += '"';
    escapeAndAppend(key);
    payload_ += "\":\"";
  }

  /* ---------------- JSON escaping ---------------- */
  void escapeAndAppend(const char *s) {
    for (; *s; ++s) {
      switch (*s) {
      case '"': payload_ += F("\\\""); break;
      case '\\': payload_ += F("\\\\"); break;
      case '\b': payload_ += F("\\b");  break;
      case '\f': payload_ += F("\\f");  break;
      case '\n': payload_ += F("\\n");  break;
      case '\r': payload_ += F("\\r");  break;
      case '\t': payload_ += F("\\t");  break;
      default:   payload_ += *s;            break;
      }
    }
  }
};
