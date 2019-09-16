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

#ifndef CODE_XFORM_EXCEPTION_HPP
#define CODE_XFORM_EXCEPTION_HPP

#include <exception>

class CodeXformException : public std::exception
{
 public:
  CodeXformException(const std::string& message)
      : m_message(message)
  {
  }

  virtual const char*
  what() const noexcept
  {
    return m_message.c_str();
  }

 private:
  std::string m_message;
};

class CodeXformSystemException : public CodeXformException {
 public:
  CodeXformSystemException(const std::string& message)
      : CodeXformException(message)
  {}
};

class CodeXformToolingException : public CodeXformException {
 public:
  CodeXformToolingException(const std::string& message)
      : CodeXformException(message)
  {}
};

class FileSystemException : public CodeXformSystemException {
 public:
  FileSystemException(const std::string& message)
      : CodeXformSystemException(message)
  {}
};

class ExecCmdException : public CodeXformSystemException {
 public:
  ExecCmdException(const std::string& cmd, const std::string& msg)
      : CodeXformSystemException(cmd + "failed with message: " + msg)
  {}
};

class CodeXformReplacementsException : public CodeXformToolingException {
 public:
  CodeXformReplacementsException(const std::string& message)
      : CodeXformToolingException(message)
  {}
};

class ConflictedReplacementsException : public CodeXformReplacementsException {
 public:
  ConflictedReplacementsException()
      : CodeXformReplacementsException("Conflicted replacements exist!")
  {}
};

class ApplyChangesException : public CodeXformReplacementsException {
 public:
  ApplyChangesException(const std::string& message)
      : CodeXformReplacementsException(message)
  {}
};

class RunClangToolException : public CodeXformException {
 public:
  RunClangToolException(const std::string& message)
      : CodeXformException(message)
  {}
};

class CommandLineOptionException : public CodeXformException {
 public:
  CommandLineOptionException(const std::string& message)
      : CodeXformException(message)
  {}
};

#endif
