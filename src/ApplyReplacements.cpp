//===-- ApplyReplacements.cpp - Apply and deduplicate replacements --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file provides the implementation for deduplicating, detecting
/// conflicts in, and applying collections of Replacements.
///
/// FIXME: Use Diagnostics for output instead of llvm::errs().
///
//===----------------------------------------------------------------------===//
#include "ApplyReplacements.hpp"
#include "MyReplacementsYaml.hpp"
#include "ToolingUtil.hpp"
#include "Logger.hpp"

#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Format/Format.h"
#include "clang/Lex/Lexer.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/DiagnosticsYaml.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace clang;

static void eatDiagnostics(const SMDiagnostic &, void *) {}

namespace clang {
namespace replace {

bool collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUReplacements &TUs,
    clang::DiagnosticsEngine &Diagnostics) {
  using namespace llvm::sys::fs;
  using namespace llvm::sys::path;

  if (extension(FilePath) != ".yaml") {
      errs() << "Error reading " << FilePath << ": file extension is not yaml" << '\n';
      return false;
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> Out =
      MemoryBuffer::getFile(FilePath);
  if (std::error_code BufferError = Out.getError()) {
      errs() << "Error reading " << FilePath << ": " << BufferError.message()
             << "\n";
      return false;
  }

  // Only keep files that properly parse.
  auto buffer = Out.get()->getBuffer();
  if (buffer.empty()) {
      // ignore empty file
      return true;
  }
  tooling::TranslationUnitReplacements TU;
  yaml::Input YIn(buffer, nullptr, &eatDiagnostics);
  if (!YIn.error()) {
      // File doesn't appear to be a header change description. Ignore it.
      YIn >> TU;
      TUs.push_back(TU);
  }
  while (YIn.nextDocument()) {
      if (!YIn.error()) {
          YIn >> TU;
          TUs.push_back(TU);
      }
  }
  return true;
}

bool collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUDiagnostics &TUs,
    clang::DiagnosticsEngine &Diagnostics) {
    using namespace llvm::sys::fs;
    using namespace llvm::sys::path;

    if (extension(FilePath) != ".yaml") {
        errs() << "Error reading " << FilePath << ": file extension is not yaml" << '\n';
        return false;
    }

    ErrorOr<std::unique_ptr<MemoryBuffer>> Out =
        MemoryBuffer::getFile(FilePath);
    if (std::error_code BufferError = Out.getError()) {
        errs() << "Error reading " << FilePath << ": " << BufferError.message()
               << "\n";
        return false;
    }
    auto buffer = Out.get()->getBuffer();
    if (buffer.empty()) {
        // ignore empty file
        return true;
    }
    yaml::Input YIn(buffer, nullptr, &eatDiagnostics);
    // Only keep files that properly parse.
    tooling::TranslationUnitDiagnostics TU;
    if (!YIn.error()) {
        // File doesn't appear to be a header change description. Ignore it.
        YIn >> TU;
        TUs.push_back(TU);
    }
    while (YIn.nextDocument()) {
        if (!YIn.error()) {
            YIn >> TU;
            TUs.push_back(TU);
        }
    }
    return true;
}

/// \brief Extract replacements from collected TranslationUnitReplacements and
/// TranslationUnitDiagnostics and group them per file. Identical replacements
/// from diagnostics are deduplicated.
///
/// \param[in] TUs Collection of all found and deserialized
/// TranslationUnitReplacements.
/// \param[in] TUDs Collection of all found and deserialized
/// TranslationUnitDiagnostics.
/// \param[in] SM Used to deduplicate paths.
///
/// \returns A map mapping FileEntry to a set of Replacement targeting that
/// file.
static llvm::DenseMap<const FileEntry *, std::vector<tooling::Replacement>>
groupReplacements(const TUReplacements &TUs, const TUDiagnostics &TUDs,
                  const clang::SourceManager &SM) {
  std::set<StringRef> Warned;
  llvm::DenseMap<const FileEntry *, std::vector<tooling::Replacement>>
      GroupedReplacements;
  // Deduplicate identical replacements in diagnostics.
  // FIXME: Find an efficient way to deduplicate on diagnostics level.
  llvm::DenseMap<const FileEntry *, std::set<tooling::Replacement>>
      DiagReplacements;
  auto AddToGroup = [&](const tooling::Replacement &R, bool FromDiag) {
    // Use the file manager to deduplicate paths. FileEntries are
    // automatically canonicalized.
    if (const FileEntry *Entry = SM.getFileManager().getFile(R.getFilePath())) {
      if (FromDiag) {
        auto &Replaces = DiagReplacements[Entry];
        if (!Replaces.insert(R).second)
          return;
      }
      GroupedReplacements[Entry].push_back(R);
    } else if (Warned.insert(R.getFilePath()).second) {
      errs() << "Described file '" << R.getFilePath()
             << "' doesn't exist. Ignoring...\n";
    }
  };
  for (const auto &TU : TUs)
    for (const tooling::Replacement &R : TU.Replacements)
      AddToGroup(R, false);

  for (const auto &TU : TUDs)
    for (const auto &D : TU.Diagnostics)
      for (const auto &Fix : D.Fix)
        for (const tooling::Replacement &R : Fix.second)
          AddToGroup(R, true);

  // Sort replacements per file to keep consistent behavior when
  // clang-apply-replacements run on differents machine.
  for (auto &FileAndReplacements : GroupedReplacements) {
    llvm::sort(FileAndReplacements.second.begin(),
               FileAndReplacements.second.end());
  }
  return GroupedReplacements;
}

bool mergeAndDeduplicate(const TUReplacements &TUs, const TUDiagnostics &TUDs,
                         FileToChangesMap &FileChanges,
                         clang::SourceManager &SM) {
  auto GroupedReplacements = groupReplacements(TUs, TUDs, SM);
  bool ConflictDetected = false;
  // To report conflicting replacements on corresponding file, all replacements
  // are stored into 1 big AtomicChange.
  for (const auto &FileAndReplacements : GroupedReplacements) {
    const FileEntry *Entry = FileAndReplacements.first;
    const SourceLocation BeginLoc =
        SM.getLocForStartOfFile(SM.getOrCreateFileID(Entry, SrcMgr::C_User));
    tooling::AtomicChange FileChange(Entry->getName(), Entry->getName());
    for (const auto &R : FileAndReplacements.second) {
      llvm::Error Err =
          FileChange.replace(SM, BeginLoc.getLocWithOffset(R.getOffset()),
                             R.getLength(), R.getReplacementText());
      if (Err) {
        // FIXME: This will report conflicts by pair using a file+offset format
        // which is not so much human readable.
        // A first improvement could be to translate offset to line+col. For
        // this and without loosing error message some modifications arround
        // `tooling::ReplacementError` are need (access to
        // `getReplacementErrString`).
        // A better strategy could be to add a pretty printer methods for
        // conflict reporting. Methods that could be parameterized to report a
        // conflict in different format, file+offset, file+line+col, or even
        // more human readable using VCS conflict markers.
        // For now, printing directly the error reported by `AtomicChange` is
        // the easiest solution.
        errs() << llvm::toString(std::move(Err)) << "\n";
        ConflictDetected = true;
      }
    }
    FileChanges.try_emplace(Entry,
                            std::vector<tooling::AtomicChange>{FileChange});
  }
  return !ConflictDetected;
}
llvm::Expected<std::string>
applyChanges(StringRef File, const std::vector<tooling::AtomicChange> &Changes,
             const tooling::ApplyChangesSpec &Spec,
             DiagnosticsEngine &Diagnostics) {
  FileManager Files((FileSystemOptions()));
  SourceManager SM(Diagnostics, Files);
  llvm::ErrorOr<std::unique_ptr<MemoryBuffer>> Buffer =
      SM.getFileManager().getBufferForFile(File);
  if (!Buffer)
    return errorCodeToError(Buffer.getError());
  return tooling::applyAtomicChanges(File, Buffer.get()->getBuffer(), Changes,
                                     Spec);
}

bool deleteReplacementFile(const llvm::StringRef FilePath,
                           clang::DiagnosticsEngine &Diagnostics) {
    bool Success = true;
    std::error_code Error = llvm::sys::fs::remove(FilePath);
    if (Error) {
        Success = false;
        // FIXME: Use Diagnostics for outputting errors.
        errs() << "Error deleting file: " << FilePath << "\n";
        errs() << Error.message() << "\n";
        errs() << "Please delete the file manually\n";
    }
    return Success;
}

bool applyReplacements(const llvm::StringRef FilePath, const llvm::StringRef Output) {
    IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
    DiagnosticsEngine Diagnostics(
        IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), DiagOpts.get());

    TUReplacements TURs;

    bool Success =
      collectReplacementsFromFile(FilePath, TURs, Diagnostics);

    TUDiagnostics TUDs;
    Success =
        collectReplacementsFromFile(FilePath, TUDs, Diagnostics);

    if (!Success) {
        errs() << "Cannot read the file '" << FilePath
               << "'\n";
        return false;
    }

    FileManager Files((FileSystemOptions()));
    SourceManager SM(Diagnostics, Files);
    
    FileToChangesMap Changes;
    if (!mergeAndDeduplicate(TURs, TUDs, Changes, SM))
        return false;

    tooling::ApplyChangesSpec Spec;

    for (const auto &FileChange : Changes) {
        const FileEntry *Entry = FileChange.first;
        StringRef FileName = Entry->getName();
        llvm::Expected<std::string> NewFileData =
            applyChanges(FileName, FileChange.second, Spec, Diagnostics);
        if (!NewFileData) {
            errs() << llvm::toString(NewFileData.takeError()) << "\n";
            continue;
        }

        // Write new file to disk
        std::error_code EC;
        if (!Output.empty()) {
            FileName = Output;
        }
        llvm::raw_fd_ostream FileStream(FileName, EC, llvm::sys::fs::F_None);
        if (EC) {
            llvm::errs() << "Could not open " << FileName << " for writing\n";
            continue;
        }
        FileStream << *NewFileData;
    }

    // Remove yaml file
    Success = deleteReplacementFile(FilePath, Diagnostics);
    return Success;
}

} // end namespace replace
} // end namespace clang
