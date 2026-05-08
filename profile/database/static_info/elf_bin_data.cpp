// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved

#define XDP_CORE_SOURCE

#include "xdp/profile/database/static_info/elf_bin_data.h"

#include <utility>

#include "core/common/message.h"

namespace xdp {

  namespace pt = boost::property_tree;
  using severity_level = xrt_core::message::severity_level;

  // Placeholder ELF custom section name carrying AIE metadata JSON.
  // Real name is TBD with the ELF builder team; until then
  // readAIEMetadata falls back to the on-disk aie_trace_config.json that
  // today's 2-arg updateDeviceFromCoreDeviceElf flow already supports.
  static constexpr const char* AIE_METADATA_ELF_SECTION = ".note.xrt.aie_metadata";

  ElfBinData::ElfBinData(xrt::elf elf, std::shared_ptr<xrt_core::device> device)
    : m_elf(std::move(elf))
    , m_device(std::move(device))
  {
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
    // Phase 1: try to read AIE metadata from a custom ELF section.
    try {
      auto span = m_elf.get_custom_section(AIE_METADATA_ELF_SECTION);
      if (span.data() && span.size()) {
        auto reader = aie::readAIEMetadata(span.data(), span.size(), out);
        if (reader) {
          xrt_core::message::send(severity_level::debug, "XRT",
            "AIE metadata read from ELF custom section.");
          return reader;
        }
      }
    }
    catch (const std::exception&) {
      // Section absent or unreadable -- fall through to disk fallback.
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
