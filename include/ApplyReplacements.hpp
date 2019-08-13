//===-- ApplyReplacements.hpp - Deduplicate and apply replacements -- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides the interface for deduplicating, detecting
/// conflicts in, and applying collections of Replacements.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_APPLYREPLACEMENTS_HPP
#define LLVM_CLANG_APPLYREPLACEMENTS_HPP

#include "clang/Tooling/Core/Diagnostic.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <system_error>
#include <vector>

namespace clang {

class DiagnosticsEngine;
class Rewriter;

namespace replace {

/// \brief Collection of TranslationUnitReplacements.
typedef std::vector<clang::tooling::TranslationUnitReplacements> TUReplacements;

/// \brief Collection of TranslationUnitReplacement files.
typedef std::vector<std::string> TUReplacementFiles;

/// \brief Collection of TranslationUniDiagnostics.
typedef std::vector<clang::tooling::TranslationUnitDiagnostics> TUDiagnostics;

/// \brief Map mapping file name to a set of AtomicChange targeting that file.
typedef llvm::DenseMap<const clang::FileEntry *,
                       std::vector<tooling::AtomicChange>>
    FileToChangesMap;

/// \brief Attempts to deserialize the given yaml file as
/// TranslationUnitReplacements. All docs that successfully deserialize are
/// added to \p TUs.
///
/// \param[in] FilePath File path to read for serialized
/// TranslationUnitReplacements.
/// \param[out] TUs Collection of all found and deserialized
/// TranslationUnitReplacements or TranslationUnitDiagnostics.
/// \param[in] Diagnostics DiagnosticsEngine used for error output.
///
/// \returns A boolean indicating success or failure in navigating the
/// directory structure. true for success and false for failure
bool collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUReplacements &TUs,
    clang::DiagnosticsEngine &Diagnostics);

bool collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUDiagnostics &TUs,
    clang::DiagnosticsEngine &Diagnostics);

/// \brief Deduplicate, check for conflicts, and extract all Replacements stored
/// in \c TUs. Conflicting replacements are skipped.
///
/// \post For all (key,value) in FileChanges, value[i].getOffset() <=
/// value[i+1].getOffset().
///
/// \param[in] TUs Collection of TranslationUnitReplacements or
/// TranslationUnitDiagnostics to merge, deduplicate, and test for conflicts.
/// \param[out] FileChanges Container grouping all changes by the
/// file they target. Only non conflicting replacements are kept into
/// FileChanges.
/// \param[in] SM SourceManager required for conflict reporting.
///
/// \returns \parblock
///          \li true If all changes were converted successfully.
///          \li false If there were conflicts.
bool mergeAndDeduplicate(const TUReplacements &TUs, const TUDiagnostics &TUDs,
                         FileToChangesMap &FileChanges,
                         clang::SourceManager &SM);

/// \brief Apply \c AtomicChange on File and rewrite it.
///
/// \param[in] File Path of the file where to apply AtomicChange.
/// \param[in] Changes to apply.
/// \param[in] Spec For code cleanup and formatting.
/// \param[in] Diagnostics DiagnosticsEngine used for error output.
///
/// \returns The changed code if all changes are applied successfully;
/// otherwise, an llvm::Error carrying llvm::StringError or an error_code.
llvm::Expected<std::string>
applyChanges(StringRef File, const std::vector<tooling::AtomicChange> &Changes,
             const tooling::ApplyChangesSpec &Spec,
             DiagnosticsEngine &Diagnostics);

/// \brief Delete the replacement file.
///
/// \param[in] File Replacement file to delete.
/// \param[in] Diagnostics DiagnosticsEngine used for error output.
///
/// \returns \parblock
///          \li true If all files have been deleted successfully.
///          \li false If at least one or more failures occur when deleting
/// files.
bool deleteReplacementFile(const llvm::StringRef File,
                           clang::DiagnosticsEngine &Diagnostics);

bool applyReplacements(const llvm::StringRef File, const llvm::StringRef Output = "");

} // end namespace replace
} // end namespace clang

#endif // LLVM_CLANG_APPLYREPLACEMENTS_HPP
