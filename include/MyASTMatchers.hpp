#ifndef MY_AST_MATCHERS_HPP
#define MY_AST_MATCHERS_HPP

#include "clang/ASTMatchers/ASTMatchers.h"

// self-defined AST Matchers and

namespace clang {
namespace ast_matchers {

// match if node is real floating type
AST_MATCHER(QualType, isRealFloating) {
    return Node->isRealFloatingType();
}

AST_MATCHER(BinaryOperator, isComparisonOperator) {
    return Node.isComparisonOp();
}

AST_MATCHER(CallExpr, hasSLSizeCmpDecl) {
    if (auto decl = Node.getDirectCallee()) {
        std::string name = decl->getNameAsString();
        if (name == "DimEqualTo" ||
            name == "DimNotEqual" ||
            name == "DimLess" ||
            name == "DimLessEqual" ||
            name == "DimGreater" ||
            name == "DimGreaterEqual") {
            return true;
        }
    }
    return false;
}

AST_MATCHER(CallExpr, hasSLSizeCastDecl) {
    if (auto decl = Node.getDirectCallee()) {
        std::string name = decl->getNameAsString();
        if (name == "DimValue2Int" ||
            name == "DimValue2Sizet" ||
            name == "DimValue2SLSize") {
            return true;
        }
    }
    return false;
}

// match if the value type is SLSize
AST_MATCHER(QualType, isSLSizeType) {
    if (Node.isNull()) return false;
    
    auto typeName = Node.getUnqualifiedType().getAsString();

    if (typeName == "SLSize" || typeName == "SLIndex") {
        return true;
    }

    return false;    
}

// match is the expr is SLSize type variable,
// value returned by vector<SLSize> index operator
AST_MATCHER(Expr, isBasicSLSizeExpr) {
    BoundNodesTreeBuilder tmpBuilder;
    if (qualType(isSLSizeType()).matches(Node.getType(), Finder, &tmpBuilder)) {
        return true;
    }
    // value returned by vector<SLSize> index operator
    else if (auto cxxOperatorCallExpr = llvm::dyn_cast<CXXOperatorCallExpr>(&Node)) {
        auto argTypeName = cxxOperatorCallExpr->getArg(0)->getType().getUnqualifiedType().getAsString();
        if (argTypeName == "vector<SLSize>" || argTypeName == "std::vector<SLSize>") return true;
    }
    return false;
}

// match if the result of the arithmetic expression is SLSize
AST_MATCHER(Expr, isArithmeticOperatorWithSLSize) {
    BoundNodesTreeBuilder tmpBuilder;
    if (auto binaryOperator = llvm::dyn_cast<BinaryOperator>(&Node))
    {
        // Node is Binaryoperator
        if (binaryOperator->isAdditiveOp() ||
            binaryOperator->isMultiplicativeOp() ||
            binaryOperator->isShiftOp()) {
            // is arithmetic operator
            if (expr(isBasicSLSizeExpr()).matches(*(binaryOperator->getLHS()->IgnoreParenImpCasts()),
                                              Finder, &tmpBuilder) ||
                expr(isBasicSLSizeExpr()).matches(*(binaryOperator->getRHS()->IgnoreParenImpCasts()),
                                              Finder, &tmpBuilder)) {
                return true;
            }
        }
    }
    if (auto unaryOperator = llvm::dyn_cast<UnaryOperator>(&Node))
    {
        // Node is Unaryoperator
        if (unaryOperator->isArithmeticOp() ||
            unaryOperator->isIncrementDecrementOp()) {
            // is arithmetic operator or IncrementDecrement operator
            if (expr(isBasicSLSizeExpr()).matches(*(unaryOperator->getSubExpr()->IgnoreParenImpCasts()),
                                              Finder, &tmpBuilder)) {
                return true;
            }
        }
    }
    if (auto callExpr = llvm::dyn_cast<CallExpr>(&Node))
    {
        // Node is call expr
        if (auto fcnDecl = callExpr->getDirectCallee()) {
            auto fcnName = fcnDecl->getNameAsString();
            if (fcnName == "max" ||
                fcnName == "min" ||
                fcnName == "std::max" ||
                fcnName == "std::min") {                
                // is max() or min()
                if (callExpr->getNumArgs() == 2 &&
                    expr(isBasicSLSizeExpr()).matches(*(callExpr->getArg(0)->IgnoreParenImpCasts()),
                                                  Finder, &tmpBuilder)) {
                    return true;
                }
            }
        }
    }
    return false;
}

// match if the expression is SLSize type
AST_MATCHER(Expr, isSLSizeExpr) {
    BoundNodesTreeBuilder tmpBuilder;
    if (expr(isBasicSLSizeExpr()).matches(Node, Finder, &tmpBuilder) ||
        expr(isArithmeticOperatorWithSLSize()).matches(Node, Finder, &tmpBuilder)) {
        return true;
    }
    return false;
}

// match if the type is int
AST_MATCHER(QualType, isIntType) {
    if (Node.isNull()) return false;
    
    auto typeName = Node.getUnqualifiedType().getAsString();
    auto canonicalTypeName = Node.getCanonicalType().getUnqualifiedType().getAsString();
    if (canonicalTypeName == "int" && typeName != "SLSize" && typeName != "SLIndex") {
        return true;
    }
    return false;
}

// match if the expr is int type
AST_MATCHER(Expr, isIntExpr) {
    BoundNodesTreeBuilder tmpBuilder;
    if (qualType(isIntType()).matches(Node.getType(), Finder, &tmpBuilder) &&
        !expr(isSLSizeExpr()).matches(Node, Finder, &tmpBuilder)) {
        return true;
    }
    return false;
}

// match if the type is size_t
AST_MATCHER(QualType, isSizetType) {
    if (Node.isNull()) return false;
    
    auto typeName = Node.getUnqualifiedType().getAsString();
    if (typeName == "size_t") {
        return true;
    }
    return false;
}

// match is the expr is size_t
AST_MATCHER(Expr, isSizetExpr) {
    BoundNodesTreeBuilder tmpBuilder;
    if (qualType(isSizetType()).matches(Node.getType(), Finder, &tmpBuilder) &&
        !expr(isSLSizeExpr()).matches(Node, Finder, &tmpBuilder)) {
        return true;
    }
    return false;
}

} // end clang namespace
} // end ast_matchers namespace

#endif
