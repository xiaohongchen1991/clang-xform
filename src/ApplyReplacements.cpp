/*
  MIT License

  Copyright (c) 2019 Xiaohong Chen

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "ApplyReplacements.hpp"
#include "MyReplacementsYaml.hpp"
#include "CoreUtil.hpp"
#include "cxxlog.hpp"
#include "CodeXformException.hpp"

#include "clang/Basic/SourceManager.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/DiagnosticsYaml.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace cxxlog;
using namespace llvm;
using namespace clang;

namespace {

void eatDiagnostics(const SMDiagnostic &, void *) {}

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

void collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUReplacements &TUs,
    clang::DiagnosticsEngine &Diagnostics) {
  using namespace llvm::sys::fs;
  using namespace llvm::sys::path;

  if (extension(FilePath) != ".yaml") {
    throw FileSystemException("Extension is not yaml for file: " + FilePath.str());
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> Out =
      MemoryBuffer::getFile(FilePath);
  if (std::error_code BufferError = Out.getError()) {
    throw FileSystemException(BufferError.message());
  }

  // Only keep files that properly parse.
  auto buffer = Out.get()->getBuffer();
  if (buffer.empty()) {
    // ignore empty file
    return;
  }

  yaml::Input YIn(buffer, nullptr, &eatDiagnostics);
  if (!YIn.error()) {
    // File doesn't appear to be a header change description. Ignore it.
    tooling::TranslationUnitReplacements TU;
    YIn >> TU;
    TUs.push_back(TU);
  }
  while (YIn.nextDocument()) {
    if (!YIn.error()) {
      tooling::TranslationUnitReplacements TU;
      YIn >> TU;
      TUs.push_back(TU);
    }
  }
}

void collectReplacementsFromFile(
    const llvm::StringRef FilePath, TUDiagnostics &TUs,
    clang::DiagnosticsEngine &Diagnostics) {
  using namespace llvm::sys::fs;
  using namespace llvm::sys::path;

  if (extension(FilePath) != ".yaml") {
    throw FileSystemException("Extension is not yaml for file: " + FilePath.str());
  }

  ErrorOr<std::unique_ptr<MemoryBuffer>> Out =
      MemoryBuffer::getFile(FilePath);
  if (std::error_code BufferError = Out.getError()) {
    throw FileSystemException(BufferError.message());
  }
  auto buffer = Out.get()->getBuffer();
  if (buffer.empty()) {
    // ignore empty file
    return;
  }
  yaml::Input YIn(buffer, nullptr, &eatDiagnostics);
  // Only keep files that properly parse.
  if (!YIn.error()) {
    // File doesn't appear to be a header change description. Ignore it.
    tooling::TranslationUnitDiagnostics TU;
    YIn >> TU;
    TUs.push_back(TU);
  }
  while (YIn.nextDocument()) {
    if (!YIn.error()) {
      tooling::TranslationUnitDiagnostics TU;
      YIn >> TU;
      TUs.push_back(TU);
    }
  }
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
      if (const auto *ChoosenFix = tooling::selectFirstFix(D)) {
        for (const auto &Fix : *ChoosenFix)
          for (const tooling::Replacement &R : Fix.second)
            AddToGroup(R, true);
      }

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

void deleteReplacementFile(const llvm::StringRef FilePath,
                           clang::DiagnosticsEngine &Diagnostics) {
  std::error_code Error = llvm::sys::fs::remove(FilePath);
  if (Error) {
    throw FileSystemException("Cannot delete file: " + FilePath.str());
  }
}

} // end of anonymous namespace

void ApplyReplacements(const llvm::StringRef FilePath, const llvm::StringRef Output) {
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
  DiagnosticsEngine Diagnostics(
      IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), DiagOpts.get());

  TUReplacements TURs;
  collectReplacementsFromFile(FilePath, TURs, Diagnostics);

  TUDiagnostics TUDs;
  collectReplacementsFromFile(FilePath, TUDs, Diagnostics);

  FileManager Files((FileSystemOptions()));
  SourceManager SM(Diagnostics, Files);

  FileToChangesMap Changes;
  if (!mergeAndDeduplicate(TURs, TUDs, Changes, SM)) {
    throw ConflictedReplacementsException();
  }

  tooling::ApplyChangesSpec Spec;

  for (const auto &FileChange : Changes) {
    const FileEntry *Entry = FileChange.first;
    StringRef FileName = Entry->getName();
    llvm::Expected<std::string> NewFileData =
        applyChanges(FileName, FileChange.second, Spec, Diagnostics);
    if (!NewFileData) {
      throw ApplyChangesException(llvm::toString(NewFileData.takeError()));
    }

    // call p4 edit
    const std::string cmd = "p4 edit " + FileName.str();
    TRIVIAL_LOG(severity::info) << "Running command: " << cmd << '\n';
    std::string p4_result;
    ExecCmd(cmd + " 2>/dev/null", p4_result);
    TRIVIAL_LOG(info) << p4_result << '\n';

    // Write new file to disk
    std::error_code EC;
    if (!Output.empty()) {
      FileName = Output;
    }
    llvm::raw_fd_ostream FileStream(FileName, EC, llvm::sys::fs::F_None);
    if (EC) {
      // swallow this error so that the other files can be processed
      llvm::errs() << "Could not open " << FileName << " for writing\n";
      continue;
    }
    FileStream << *NewFileData;
  }

  // Remove yaml file
  deleteReplacementFile(FilePath, Diagnostics);
}
