// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved

#define XDP_CORE_SOURCE

#include "xdp/profile/database/static_info/elf_bin_data.h"

#include <utility>

#include "core/common/message.h"
#include "xdp/profile/database/static_info/aie_constructs.h"

namespace xdp {

  namespace pt = boost::property_tree;
  using severity_level = xrt_core::message::severity_level;


  ElfBinData::ElfBinData(xrt::elf elf, std::shared_ptr<xrt_core::device> device)
    : m_elf(std::move(elf))
    , m_device(std::move(device))
  {
  }

  ElfBinData::~ElfBinData()
  {
    for (auto* c : m_aieList)
      delete c;
    m_aieList.clear();
  }

  void
  ElfBinData::addAIECounter(uint32_t i, uint8_t col, uint8_t row, uint8_t num,
                            uint16_t start, uint16_t end, uint8_t reset,
                            uint64_t load, double freq, const std::string& mod,
                            const std::string& aieName, uint8_t streamId)
  {
    m_aieList.push_back(new AIECounter(i, col, row, num, start, end, reset,
                                       load, freq, mod, aieName, streamId));
  }

  uint64_t
  ElfBinData::numAIECounters() const
  {
    return m_aieList.size();
  }

  AIECounter*
  ElfBinData::getAIECounter(uint64_t idx) const
  {
    if (idx >= m_aieList.size())
      return nullptr;
    return m_aieList[idx];
  }

  xrt_core::uuid
  ElfBinData::uuid() const
  {
    try {
      return m_elf.get_cfg_uuid();
    }
    catch (const std::exception&) {
      return xrt_core::uuid();
    }
  }

  std::string
  ElfBinData::name() const
  {
    return std::string("elf");
  }

  std::unique_ptr<aie::BaseFiletypeImpl>
  ElfBinData::readAIEMetadata(pt::ptree& out)
  {
    // Phase 1: read AIE metadata from the AIE_TRACE_METADATA ELF custom
    // section.  This is the source of truth for the Full ELF flow.
    try {
      auto data = m_elf.get_custom_section("AIE_TRACE_METADATA");
      if (data.data() && data.size()) {
        auto reader = aie::readAIEMetadata(data.data(), data.size(), out);
        if (reader) {
          xrt_core::message::send(severity_level::debug, "XRT",
            "AIE metadata read from ELF custom section.");
          return reader;
        }
      }
    }
    catch (const std::exception& e) {
      std::string msg = "AIE metadata ELF custom section unavailable: ";
      msg += e.what();
      xrt_core::message::send(severity_level::debug, "XRT", msg);
      // Fall through to disk fallback.
    }

    // Phase 2: disk-JSON fallback, mirroring the 2-arg ELF overload.
    auto reader = aie::readAIEMetadata("aie_trace_config.json", out);
    if (reader) {
      xrt_core::message::send(severity_level::debug, "XRT",
        "AIE metadata read from disk (aie_trace_config.json) for ELF flow.");
    }
    else {
      xrt_core::message::send(severity_level::debug, "XRT",
        "AIE metadata not available for ELF flow.");
    }
    return reader;
  }

  void
  ElfBinData::populateFromReader(const aie::BaseFiletypeImpl& reader)
  {
    try {
      m_aieClockRateMHz = reader.getAIEClockFreqMHz();
    }
    catch (const std::exception&) {
      // Keep default clock rate if the reader cannot report one.
    }
    try {
      m_aieGeneration = static_cast<uint8_t>(reader.getHardwareGeneration());
    }
    catch (const std::exception&) {
      // Keep default generation if the reader cannot report one.
    }
  }

} // namespace xdp
