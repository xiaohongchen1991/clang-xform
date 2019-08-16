# Release versions

version 1.0.0:

clang-xform - a simple Clang-based app for large-scale C++ code transformation

# Description

**clang-xform** is a clang-based app that automatically performs predefined transformation on C++ source code. It can match certain code pattern and apply specified transformation through the Clang AST matcher and its callback function registered by the user. The predefined AST matchers are listed and explained in [Matcher list](#matcher-list). The [Quick tutorial](#quick-tutorial) will demonstrate how to quickly authorize your own AST matchers and apply them in your codebase. More information can be found in the resources listed in [Resources](#resources).

Author/contact: Xiaohong Chen, xiaohong_chen1991@hotmail.com

# Quick tutorial

This tutorial is aimed to demonstrate how to write your own Clang AST matcher and apply it in your codebase. Let's suppose you want to write a matcher to match all the callsites of function "*Foo*" and then rename them as "*Bar*". Typical workflow:

1. Clone the git repository

```
git clone URL
```

2. Let's say the matcher ID is "RenameFoo". One can create a matcher template file by using

```
cd clang-xform/src/matchers/rename
../../../scripts/gen-matcher.py RenameFoo
```

3. A cpp file named "RenameFoo.cpp" is created in the current working directory. Edit it following the instructions in the file. In RenameFoo.cpp, you only need to edit the definitions of the matcher and its callback function. The following is the code snippet of the generated template file.

```cpp
// This a generated template file to help write your own clang AST matcher and callback.
// Please address all comments in /**/ form below!
// [1] : use StatementMatcher or DeclarationMatcher
// [2] : replace it with a suitable node matcher
// [3] : add narrowing or traversal matchers here
// [4] : rename the string ID for a better description of the matched node
// [5] : replace it with the corresponding node class type binded with the string ID
// [6] : variable name for the matched node
// [7] : same as [5]
// [8] : string ID used in matcher, same as [4]
// [9] : name of the node to be replaced
// [10]: string used to replace the matched node
// [11]: use "LogASTNode(locStart, srcMgr, oldExprString)" to log matched AST Node infomation
// [12]: can register more than one matcher of different types

StatementMatcher/*[1]*/ RenameFooMatcher =
    expr/*[2]*/(/*[3]*/,
                isExpansionInMainFile()
        ).bind("RenameFooExpr"/*[4]*/);

// Match callback class RenameFooCallback is defined here
MATCH_CALLBACK(RenameFooCallback);

// Defination of RenameFooCallback::run
void RenameFooCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
    std::string oldExprString;
    std::string newExprString;
    const auto& srcMgr = Result.Context->getSourceManager();
    const auto& langOpts = Result.Context->getLangOpts();

    // Check any AST node matched for the given string ID.
    // The node class name is usually the capitalized node matcher name.
    if (const Expr*/*[5]*/ RenameFooExpr/*[6]*/ =
        Result.Nodes.getNodeAs<Expr/*[7]*/>("RenameFooExpr"/*[8]*/)) {
        // find begin and end file locations of a given node
        auto locStart = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getBeginLoc());
        auto locEnd = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getEndLoc());
        newExprString = ""/*[10]*/;
        // find source text for a given location
        oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
        // replace source text with a given string
        ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString);
        // log the replacement or AST node if no replacement is made
        LogReplacement(locStart, srcMgr, oldExprString, newExprString)/*[11]*/;
    }
}
```

In this example, we only need to edit locations [2], [3], [5], [7], [9], and [10]. The new code looks like

```cpp
StatementMatcher/*[1]*/ RenameFooMatcher =
    callExpr/*[2]*/(/*[3]*/callee(functionDecl(hasName("Foo"))),
                isExpansionInMainFile()
        ).bind("RenameFooExpr"/*[4]*/);

// Match callback class RenameFooCallback is defined here
MATCH_CALLBACK(RenameFooCallback);

// Defination of RenameFooCallback::run
void RenameFooCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
    std::string oldExprString;
    std::string newExprString;
    const auto& srcMgr = Result.Context->getSourceManager();
    const auto& langOpts = Result.Context->getLangOpts();

    // Check any AST node matched for the given string ID.
    // The node class name is usually the capitalized node matcher name.
    if (const CallExpr*/*[5]*/ RenameFooExpr/*[6]*/ =
        Result.Nodes.getNodeAs<CallExpr/*[7]*/>("RenameFooExpr"/*[8]*/)) {
        // find begin and end file locations of a given node
        auto locStart = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getCallee()->getExprLoc());
        auto locEnd = srcMgr.getFileLoc(RenameFooExpr/*[9]*/->getCallee()->getEndLoc());
        newExprString = "Bar"/*[10]*/;
        // find source text for a given location
        oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
        // replace source text with a given string
        ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString);
        // log the replacement or AST node if no replacement is made
        LogReplacement(locStart, srcMgr, oldExprString, newExprString)/*[11]*/;
    }
}
```

4. Rebuild the tool.

```
cd clang-xform
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

where "compile_commands.json" file contains complilation database for all the files you want to refactor.

8. Writing a unit test for each new added matcher is recommended. A unit test framework is set up for the users to easily add their tests. For more information, see [Testing](#testing).


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
cd clang-xform/src/matchers

# create a new folder to store your files
mkdir rename

# use the helper script to generate a matcher file template
cd rename
../../../scripts/gen-matcher.py RenameFcn  # RenameFcn.cpp is generated in the current folder
```

Now one can edit RenameFcn.cpp following the instructions in the file. After rebuilding the tool, the new matcher can be selected by "--matchers RenameFcn".

See [Matcher list](matcher-list) for the information about predefined matchers.

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
clang-xform -m RenameFcn -p compile_commands.json -f File
```

## -- [CLANG\_FLAGS]

Optional argument separator used to specify compilation flags for internal clang front-end. This is useful when testing ast_matcher on a simple test file independent from Simulink code base. e.g.

```
# -g sets debug mode for clang
clang-xform -m DimCastRm FILE -- -g
```

# Testing

This tool, in general, is used for pattern match and code refactoring. So the test strategy is to compare both the log file and the refactored file with their corresponding baseline. The log file contains matched file, line, code context, and is ideal to be used for testing when the tool is used for pattern match. The refactored file comparison is mainly used for code refactoring testing purpose. The following is the general procedure to add unit test for the new matcher "RenameFoo" demonstrated in [Quick tutorial](quick-tutorial).

1. Create a src file to be refactored by the matcher. Here, we choose to use default name "example.cpp".

```
cd clang-xform/test/rename
mkdir RenameFoo
cd RenameFoo
emacs example.cpp
```

2. Generate unit test file and baselines by using the script gen-test.py

```
../../../gen-test.py -m RenameFoo -l clang-xform.log example.cpp
```

This will generate a gtest file named "tRenameFoo.cpp" and two baseline, namely "example.cpp.gold" and "clang-xform.log.gold".

3. Rebuild the tool by

```
cd clang-xform
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

# Linking

Clang 8.0.0 is required. Clang prebuilt binaries are available at http://releases.llvm.org/download.html 

# Requirements

This tool requires Clang 8.0.0 and a C++ compiler that supports C++14. Linux system is also required for now.

# TODO list

* Support command line options for matcher
* Support windows system
