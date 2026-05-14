// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved

#ifndef ELF_BIN_DATA_DOT_H
#define ELF_BIN_DATA_DOT_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

#include "core/common/system.h"
#include "core/include/xrt/experimental/xrt_elf.h"

#include "xdp/config.h"
#include "xdp/profile/database/static_info/aie_util.h"
#include "xdp/profile/database/static_info/filetypes/base_filetype_impl.h"

namespace xdp {

  // Forward declaration: full definition lives in aie_constructs.h, which
  // we deliberately avoid including from this header to keep the public
  // surface narrow.  Implementation file pulls in the full type.
  struct AIECounter;

  // POC-local enum that identifies the source of binary metadata.  Will
  // be promoted into a shared vp_bin_data.h header once the abstract
  // VPBinData interface is introduced (with XclBinData as the xclbin-side
  // derivation and ElfBinData remaining for the ELF flow).
  enum class VPBinDataKind { Xclbin, Elf };

  // ElfBinData is the per-device AIE state container for the Full ELF
  // flow.  It is the ELF-flow analog of XclbinInfo: every value the
  // VPStaticDatabase exposes for an ELF-loaded device (AIE clock rate,
  // AIE generation, "have we configured counters yet?" flag, identity)
  // lives here.  The class is intentionally AIE-only -- a Full ELF
  // carries no PL section -- so PL-shaped accessors are absent by design.
  //
  // For the POC this class is standalone (no inheritance).  In the
  // upcoming refactor it will derive from a shared VPBinData base
  // alongside XclBinData; the public surface here is shaped so that
  // promotion is mechanical.
  class ElfBinData
  {
  public:
    XDP_CORE_EXPORT
    ElfBinData(xrt::elf elf, std::shared_ptr<xrt_core::device> device);

    XDP_CORE_EXPORT ~ElfBinData();

    // Identity ---------------------------------------------------------
    VPBinDataKind  kind() const { return VPBinDataKind::Elf; }
    XDP_CORE_EXPORT xrt_core::uuid uuid() const;
    XDP_CORE_EXPORT std::string    name() const;

    // AIE state -- the ELF analog of XclbinInfo::aie / AIEInfo --------
    double  aieClockRateMHz() const     { return m_aieClockRateMHz; }
    uint8_t aieGeneration()   const     { return m_aieGeneration; }
    bool    isAIECounterRead() const    { return m_aieCounterRead; }
    void    setIsAIECounterRead(bool v) { m_aieCounterRead = v; }

    // AIE counter list -- the ELF analog of AIEInfo::aieList.  The
    // xclbin path stores reserved AIE perf counters on the active
    // XclbinInfo; the ELF path has no XclbinInfo, so the same list
    // lives here.  Raw pointers match the xclbin-side ownership model
    // (deleted in the destructor below).
    XDP_CORE_EXPORT
    void addAIECounter(uint32_t i, uint8_t col, uint8_t row, uint8_t num,
                       uint16_t start, uint16_t end, uint8_t reset,
                       uint64_t load, double freq, const std::string& mod,
                       const std::string& aieName, uint8_t streamId = 0);

    XDP_CORE_EXPORT uint64_t numAIECounters() const;
    XDP_CORE_EXPORT AIECounter* getAIECounter(uint64_t idx) const;

    // Acquire AIE metadata.  Tries the AIE_TRACE_METADATA custom
    // section embedded in the ELF first, then falls back to disk-JSON
    // (aie_trace_config.json) matching today's 2-arg ELF flow.  Returns
    // the produced filetype reader so the caller can register it on
    // the database's metadata-reader map, or nullptr if neither source
    // is available.  No XclbinInfo touched.
    XDP_CORE_EXPORT
    std::unique_ptr<aie::BaseFiletypeImpl>
    readAIEMetadata(boost::property_tree::ptree& out);

    // Cache the AIE state derivable from a metadata reader so subsequent
    // database lookups (clock rate, hw gen) can answer without re-parsing.
    XDP_CORE_EXPORT
    void populateFromReader(const aie::BaseFiletypeImpl& reader);

  private:
    xrt::elf m_elf;
    std::shared_ptr<xrt_core::device> m_device;

    // Defaults match AIEInfo so the ELF path returns the same values
    // the xclbin path does when metadata is absent.
    double  m_aieClockRateMHz = 1000.0;
    uint8_t m_aieGeneration   = 1;
    bool    m_aieCounterRead  = false;

    // Reserved AIE perf counters for this ELF-loaded device.
    std::vector<AIECounter*> m_aieList;
  };

} // namespace xdp

#endif
