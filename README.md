# Release versions

version 1.1.0:

* matcher supports its own command line arguments

* gen-test.py supports generating test for matchers with command line options

* matcher "RenameFcn" is modified to support command line options

* support new flag "-v, --version" to display version number

# Description

**clang-xform** is a clang-based app that automatically performs predefined transformation on C++ source code. It can match certain code pattern and apply specified transformation through the Clang AST matcher and its callback function registered by the user. The predefined AST matchers are listed and explained in [Matcher list](#matcher-list). The [Quick tutorial](#quick-tutorial) will demonstrate how to quickly authorize your own AST matchers and apply them in your codebase.
Author/contact: Xiaohong Chen, xiaohong_chen1991@hotmail.com

# Quick tutorial

This tutorial is aimed to demonstrate how to write your own Clang AST matcher and apply it in your codebase. Let's suppose you want to write a matcher to match all the callsites of function "*Foo*" and then rename them as "*Bar*". Typical workflow:

1. Clone the git repository

```
git clone URL
```

2. Let's say the matcher ID is "RenameFoo". One can create a matcher template file by using

```
cd INSTALL_DIR/clang-xform/src/matchers/rename
../../../scripts/gen-matcher.py RenameFoo
```

3. A cpp file named "RenameFoo.cpp" is created in the current working directory. Edit it following the instructions in the file. In RenameFoo.cpp, you only need to edit the definitions of the matcher and its callback function. The following is the code snippet of the generated template file.

```cpp
// This is a generated template file to help write your own clang AST matcher and callback.
// If you want to support command line options, use -o, --option flag for gen-option-matcher.py.
// Please address all comments in /**/ form below!
// [1] : use StatementMatcher or DeclarationMatcher
// [2] : replace it with a suitable node matcher
// [3] : add narrowing or traversal matchers here
// [4] : rename the string ID for a better description of the matched node
// [5] : replace it with the corresponding node class type bound with the string ID
// [6] : variable name for the matched node
// [7] : same as [5]
// [8] : string ID used in matcher, same as [4]
// [9] : name of the node to be replaced
// [10]: string used to replace the matched node
// [11]: Use ReplaceText() for replacement and InsertText() for insertion (ex. header insertion)
// [12]: use "LogASTNode(locStart, srcMgr, oldExprString)" to log matched AST Node information

namespace {

// Match callback class RenameFooCallback is defined here
MATCH_CALLBACK(RenameFooCallback);

void RenameFooCallback::RegisterMatchers(clang::ast_matchers::MatchFinder* finder) {
  // Define your own matcher here.
  // Use StatementMatcher to match statements and DeclarationMatcher to match declarations.
  // It is recommended that isExpansionInMainFile() is used to avoid matches in
  // system headers or third-party libraries.
  // For AST matcher reference, see: https://clang.llvm.org/docs/LibASTMatchersReference.html
  // For AST matcher examples, check files under "matchers/" directory
  StatementMatcher/*[1]*/ RenameFooMatcher =
      expr/*[2]*/(/*[3]*/,
                  isExpansionInMainFile()
                  ).bind("RenameFooExpr"/*[4]*/);

  finder->addMatcher(RenameFooMatcher, this);
}

// Definition of RenameFooCallback::run
void RenameFooCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  std::string oldExprString;
  std::string newExprString;
  const auto& srcMgr = Result.Context->getSourceManager();
  const auto& langOpts = Result.Context->getLangOpts();

  // Check any AST node matched for the given string ID.
  // The node class name is usually the capitalized node matcher name.
  if (const Expr* /*[5]*/ RenameFooExpr/*[6]*/ =
      Result.Nodes.getNodeAs<Expr/*[7]*/>("RenameFooExpr"/*[8]*/)) {
    // find begin and end file locations of a given node
    auto locStart = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getBeginLoc());
    auto locEnd = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getEndLoc());
    newExprString = ""/*[10]*/;
    // find source text for a given location
    oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
    // replace source text with a given string or use InsertText() to insert new text
    ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString)/*[11]*/;
    // log the replacement or AST node if no replacement is made
    LogReplacement(locStart, srcMgr, oldExprString, newExprString)/*[12]*/;
  }
}
```

In this example, we only need to edit locations [2], [3], [5], [7], [9], and [10]. The new code looks like

```cpp
// Match callback class RenameFooCallback is defined here
MATCH_CALLBACK(RenameFooCallback);

void RenameFooCallback::RegisterMatchers(clang::ast_matchers::MatchFinder* finder) {
  StatementMatcher/*[1]*/ RenameFooMatcher =
      callExpr/*[2]*/(callee(functionDecl(hasName("Foo")))/*[3]*/,
                      isExpansionInMainFile()
                      ).bind("RenameFooExpr"/*[4]*/);

  finder->addMatcher(RenameFooMatcher, this);
}

// Definition of RenameFooCallback::run
void RenameFooCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  std::string oldExprString;
  std::string newExprString;
  const auto& srcMgr = Result.Context->getSourceManager();
  const auto& langOpts = Result.Context->getLangOpts();

  // Check any AST node matched for the given string ID.
  // The node class name is usually the capitalized node matcher name.
  if (const CallExpr* /*[5]*/ RenameFooExpr/*[6]*/ =
      Result.Nodes.getNodeAs<CallExpr/*[7]*/>("RenameFooExpr"/*[8]*/)) {
    // find begin and end file locations of a given node
    auto locStart = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getCallee()->getExprLoc());
    auto locEnd = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getCallee()->getEndLoc());
    newExprString = "Bar"/*[10]*/;
    // find source text for a given location
    oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
    // replace source text with a given string or use InsertText() to insert new text
    ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString)/*[11]*/;
    // log the replacement or AST node if no replacement is made
    LogReplacement(locStart, srcMgr, oldExprString, newExprString)/*[12]*/;
  }
}
```

4. Rebuild the tool.

```
cd INSTALL_DIR/clang-xform
make
```

5. Now you can apply your tool in your codebase. First check if your matcher is registered by

```
bin/clang-xform -d
```

This will print a list of registered matchers. You should find "RenameFoo" among them.

6. The file "clang-xform/test/rename/RenameFcn/example.cpp" contains callsites of function "*Foo*". We can first run the matcher on this file to give a try.

```
bin/clang-xform -m RenameFoo -o output.yaml test/rename/RenameFcn/example.cpp
```

The logging information will be displayed on the screen and saved in clang-xform.log in the current working directory. Using switch "-l, --log FILE.log" can change the default log file. Open the log file clang-xform.log and check logged replacements. If everything looks fine, we can apply these replacements by

```
bin/clang-xform -a output.yaml
```

Note that the replacements stored in output.yaml may duplicate and even conflict each other. The tool will ignore these duplicates and conflicts.

7. Now, we can run the tool in your codebase by

```
bin/clang-xform --compile-commands compile_commands.json
```

where "compile_commands.json" file contains compilation database for all the files you want to refactor.

8. This matcher can be reused to rename any function callsites. Since the qualified name and new name are hard-coded in the matcher, it is quite inconvenient to edit the code and rebuild it again. Instead, we can supply this information in the command line arguments. To do this, we first use "-o, --option" in gen-matcher.py to regenerate a template file.

```
cd INSTALL_DIR/clang-xform/src/matchers/rename
../../../scripts/gen-matcher.py -o RenameFoo
```

This create a "RenameFcn.cpp" file. Then redo the previous steps. The final version has checked in to the repository. The following is part of the code snippet different from the original one.

```cpp
// option1: qualified function name to match
// option2: new function name to use
const std::string option1 = "qualified-name";
const std::string option2 = "new-name";

// Match callback class RenameFcnCallback is defined here
OPTION_MATCH_CALLBACK(RenameFcnCallback);

void RenameFcnCallback::RegisterOptions() {
  AddOption<std::string>(option1);
  AddOption<std::string>(option2);
}
```

As you can see, MACRO "MATCH\_CALLBACK" is replaced with "OPTION\_MATCH\_CALLBACK", and the member function "RegisterOptions()" is added where user can register different types of options. To retrieve the command arguments, use

```cpp
std::string value1 = GetOption<std::string>(option1);
```

Please check this file in the repository for more details. To use this matcher, one has to supply these matcher options in command line. For example,

```
cd INSTALL_DIR/clang-xform
bin/clang-xform -m RenameFcn -o output.yaml -f test/rename/RenameFcn/example.cpp \
--matcher-args-RenameFcn --qualified-name Foo --new-name Bar
```

Note that, in this case, input-files argument can not be positional when using with matcher arguments. Here "--matcher-args-RenameFcn" is a separator used to tell parser that the arguments after it and before the end or the next separator are used for matcher RenameFcn. For more information, see [Switches and arguments](#switches-and-arguments).

9. Writing a unit test for each new added matcher is recommended. A unit test framework is set up for the users to easily add their tests. For more information, see [Testing](#testing).


# Switches and arguments
```
sbcodexform
  -h, --help                                    # produce help message
  -a, --apply FILE.yaml                         # apply replacements
  -j, --num-threads N                           # number of threads, default all cores
  -p, --compile-commands compile_commands.json  # read compile commands for clang
  -m, --matchers "MATCHER1 ..."                 # select matchers to apply
  -o, --output FILE.yaml                        # export replacements suggestions in yaml file
  -d, --display                                 # display registered matchers
  -q, --quiet                                   # silent output in the terminal
  -l, --log FILE.log                            # log file name
  -f, --input-files "FILE1 ..."                 # files to refactor
  --matcher-args-MATCHER_NAME [MATCHER_ARGS]    # arguments for registered matcher options
  -- [CLANG_FLAGS]                              # optional argument separator
```

## -a, --apply FILE.yaml

Specify the replacement file to apply. The extension of the supplied file must be yaml.

## -j, --num-threads N

Number of cores to use. The default is all logical cores. Also, each core will run at least three files. So if there are only two files to refactor, no additional threads will be created.

## -m, --matchers "MATCHER1 ..."

One or more matchers to apply. New matchers can be registered in cpp files under the folder "clang-xform/src/matchers". For example, to create a matcher for function renaming, one can do the following steps.

```
# cd to matchers folder
cd INSTALL_DIR/clang-xform/src/matchers

# create a new folder to store your files
mkdir rename

# use the helper script to generate a matcher file template
cd rename
../../../scripts/gen-matcher.py RenameFcn  # RenameFcn.cpp is generated in the current folder
```

Now one can edit RenameFcn.cpp following the instructions in the file. After rebuilding the tool, the new matcher can be selected by "--matchers RenameFcn".

See [Matcher list](#matcher-list) for the information about predefined matchers.

## -p, --compile-commands compile_commands.json

To refactor specified files, one has to provide compilation flags used by internal clang front-end. One way to do it is to read those flag from a specified "compile_commands.json" file.

## -o, --output FILE.yaml

Specify the output yaml file to store generated replacement suggestions. One has to manually apply those replacements after the tool finishes using "-a, --apply FILE.yaml". If this switch is not provided, a temporary file "tmp.yaml" will be created and the tool will apply those replacements in "tmp.yaml" automatically at the end.

## -d, --display

Print a list of matcher IDs registered in the tool.

## -q, --quiet

Setting this switch will silent the log information output in the terminal screen. The log information can still be found in the log file.

## -l, --log FILE.log

Specify log file to store logging information. It is an optional switch. By default, "clang-xform.log" in the current working directory is used.

## -f, --input-files "FILE1 ..."

One or more files to be refactored. This switch is a positional argument, which means you can directly specifies these files at the end of command line. i.e.

```
clang-xform -m RenameFcn File -- [CLANG_FLAGS]
```

When using with "--matcher-args-MATCHER\_NAME [MATCHER_ARGS]", the parser cannot tell if the supplied files are positional arguments. In this case, switch "-f, --input-files" has to be used. i.e.

```
clang-xform -m RenameFcn -p compile_commands.json -f File
```

## --matcher-args-MATCHER\_NAME [MATCHER\_ARGS]

Optional arguments for registered matcher options. Here "--matcher-args-Matcher_Name" serves as a separator to tell the parser that the arguments after it and before the next separator are used for the matcher with the given name. This switch has to be used at the end of command line or before "--" if "--" is used for supplying Clang flags.

## -- [CLANG\_FLAGS]

Optional argument separator used to specify compilation flags for internal clang front-end. If used, it is required to place them at the end of the command line. This is useful when testing ast_matcher on a simple test file. e.g.

```
# -g sets debug mode for clang
clang-xform -m DimCastRm FILE -- -g
```

# Testing

This tool, in general, is used for pattern match and code refactoring. So the test strategy is to compare both the log file and the refactored file with their corresponding baseline. The log file contains matched file, line, code context, and is ideal to be used for testing when the tool is used for pattern match. The refactored file comparison is mainly used for code refactoring testing purpose. The following is the general procedure to add unit test for the new matcher "RenameFoo" demonstrated in [Quick tutorial](#quick-tutorial).

1. Create a src file to be refactored by the matcher. Here, we choose to use default name "example.cpp".

```
cd INSTALL_DIR/clang-xform/test/rename
mkdir RenameFoo
cd RenameFoo
emacs example.cpp
```

2. Generate unit test file and baselines by using the script gen-test.py

```
../../../gen-test.py -m RenameFoo -l clang-xform.log -p LLVM_ROOT example.cpp
```

This will generate a gtest file named "tRenameFoo.cpp" and two baseline, namely "example.cpp.gold" and "clang-xform.log.gold". Note that the argument "LLVM\_ROOT" for the switch "-p, --path" refers to the LLVM library root path. This path will be used to generate compile\_commands.json file for the matcher unit test. If the matcher has its own command line options, swtich "-a, --arguments" is required to supply the arguments in a quoted string. e.g.

```
../../../gen-test.py -m RenameFcn -l clang-xform.log -p LLVM_ROOT\
-a "--qualified-name Foo --new-name Bar" example.cpp
```


3. Rebuild the tool by

```
cd INSTALL_DIR/clang-xform
make
```

4. To run the tests, do

```
make test
```

Then, the refactored src file "example.cpp.refactored" and log file "clang-xform.log" will be generated by the tool and will be used to compare with their corresponding baseline file.

# Matcher list

## Rename

* RenameFcn: rename callsites of a certain function by another specified name

```
RenameFcn
  --qualified-name                              # qualified name to match
  --new-name                                    # new name used to replace matched name
```

# FAQ

## Q1. What is the difference between "Replacement" and "Insertion"?

Two sets of public APIs are provided in "MatchCallbackBase.hpp" to replace or insert text. i.e.

```cpp
// replace context specified by a starting source location and length with the given string
llvm::Error MatchCallbackBase::ReplaceText(const clang::SourceManager&,
	                                       clang::SourceLocation,
                                           unsigned,
                                           llvm::StringRef);
                                            
// replace context specified by a range of source locations with the given string
llvm::Error MatchCallbackBase::ReplaceText(const clang::SourceManager&,
                                           clang::SourceRange,
                                           llvm::StringRef,
                                           const clang::LangOptions&);

// insert string in the specified source location
void InsertText(const clang::SourceManager&,
                clang::SourceLocation,
                llvm::StringRef);
```

The major difference here is that InsertText() allows duplications and conflicts. For example, one should be allowed to insert multiple header includes in the same source location. In contrast, ReplaceText() will discard conflicts (e.g. overlapping source ranges) and most duplicates (e.g. same replacement). The only exception happens when using ReplaceText() to do insertion (source range length is zero). In this case, duplicates will be merged (like using InsertText()) instead of discarded. 

## Q2. How to insert a new header include in the file?

An API is provided in "MatchCallbackBase.hpp" to insert header includes. i.e.

```
// insert a new header include in the file specified by fileID and
// group header includes matching the given regex
llvm::Optional<clang::SourceLocation> InsertHeader(const clang::SourceManager& srcMgr,
                                                   const clang::FileID& fileID,
                                                   llvm::StringRef header,
                                                   llvm::StringRef regex);
```

This API will first look up for the given header in the file. If it is already existing, the API will simply return. Otherwise the header include will be inserted in the following way.

1. If there exists header include matching the given regex, insert there.

2. Otherwise, insert the new header include after user-defined headers (include "...") and before system and third-party library headers (include <...>).

Here is an example. Let's say we want to include a new header "Foo/foo.hpp". We can set regex to be "Foo". The followings are some possible results.

```
// file1
#include "Foo/foo2.hpp"
#include "Foo/foo.hpp"
#include "Bar/bar1.hpp"
#include "Bar/bar2.hpp"

#include <vector>
#include <memory>

// file2
#include "Bar/bar1.hpp"
#include "Bar/bar2.hpp"
#include "Foo/foo.hpp"

#include <vector>
#include <memory>
```

Note that, when inserting new header includes, it is best for your matcher to match each translation unit only once (remove isExpansionInMainFile from generated template matcher) to avoid including the same header multiple times.

# Linking

Clang 9.0.0 is required. Clang prebuilt binaries are available at http://releases.llvm.org/download.html

# Requirements

This tool requires Clang 9.0.0 and a C++ compiler that supports C++14. Linux system is also required for now.

# TODO list

* Switch from error code into using exception handling
* Support windows system
