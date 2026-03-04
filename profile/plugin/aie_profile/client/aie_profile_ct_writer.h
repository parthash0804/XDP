// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2025-2026 Advanced Micro Devices, Inc. All rights reserved

#ifndef AIE_PROFILE_CT_WRITER_NPU3_H
#define AIE_PROFILE_CT_WRITER_NPU3_H

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace xdp {

class VPDatabase;
class AieProfileMetadata;
struct AIECounter;

struct SaveTimestampInfo {
  uint32_t lineNumber;
  int optionalIndex;
};

struct CTCounterInfo {
  uint8_t column;
  uint8_t row;
  uint8_t counterNumber;
  std::string module;
  uint64_t address;
  std::string metricSet;
  std::string portDirection;
};

struct ASMFileInfo {
  std::string filename;
  int asmId;
  int ucNumber;
  int colStart;
  int colEnd;
  std::vector<SaveTimestampInfo> timestamps;
  std::vector<CTCounterInfo> counters;
};

class AieProfileCTWriter {
public:
  AieProfileCTWriter(VPDatabase* database,
                     std::shared_ptr<AieProfileMetadata> metadata,
                     uint64_t deviceId);
  ~AieProfileCTWriter() = default;

  bool generate();

private:
  std::vector<ASMFileInfo> readASMInfoFromCSV(const std::string& csvPath);
  std::vector<CTCounterInfo> getConfiguredCounters();
  std::vector<CTCounterInfo> filterCountersByColumn(
      const std::vector<CTCounterInfo>& allCounters,
      int colStart, int colEnd);
  uint64_t calculateCounterAddress(uint8_t column, uint8_t row,
                                   uint8_t counterNumber,
                                   const std::string& module);
  bool writeCTFile(const std::vector<ASMFileInfo>& asmFiles,
                   const std::vector<CTCounterInfo>& allCounters);
  std::string formatAddress(uint64_t address);
  uint64_t getModuleBaseOffset(const std::string& module);
  bool isThroughputMetric(const std::string& metricSet);
  std::string getPortDirection(const std::string& metricSet, uint64_t payload);

  VPDatabase* db;
  std::shared_ptr<AieProfileMetadata> metadata;
  uint64_t deviceId;

  uint8_t columnShift;
  uint8_t rowShift;

  // NPU3 base offsets (from npu3_registers.h)
  static constexpr uint64_t CORE_MODULE_BASE_OFFSET   = 0x000A2530;
  static constexpr uint64_t MEMORY_MODULE_BASE_OFFSET = 0x00000020;
  static constexpr uint64_t MEM_TILE_BASE_OFFSET      = 0x00000020;
  static constexpr uint64_t SHIM_TILE_BASE_OFFSET     = 0x00004020;

  static constexpr const char* CT_OUTPUT_FILENAME = "aie_profile.ct";
};

} // namespace xdp

#endif // AIE_PROFILE_CT_WRITER_NPU3_H
