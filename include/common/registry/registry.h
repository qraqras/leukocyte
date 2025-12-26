#ifndef LEUKOCYTE_RULE_REGISTRY_H
#define LEUKOCYTE_RULE_REGISTRY_H

/* DEPRECATION NOTICE ⚠️
 * The legacy flat rule registry API has been removed in favor of the
 * category-indexed generated registry (`include/common/generated_rules.h`).
 * Please migrate callers to use:
 *   - `leuko_get_rule_categories()`
 *   - `leuko_get_rule_category_count()`
 *   - `leuko_rule_find_index(category, name)`
 *
 * This header remains to provide legacy rule name macros; it will be removed
 * in a future major version.
 */

#warning "include/common/registry/registry.h is deprecated; include common/generated_rules.h instead"

#include <stddef.h>
#include "common/category.h"
#include "configs/common/rule_config.h"

/**
 * @brief Rule structure forward declaration.
 */
typedef struct rule_s rule_t;

#define LEUKO_FULLNAME(cat, sname) cat "/" sname

/* Layout Rules */
/* clang-format off */
#define LEUKO_RULE_NAME_ACCESS_MODIFIER_INDENTATION                    "AccessModifierIndentation"
#define LEUKO_RULE_NAME_ARGUMENT_ALIGNMENT                             "ArgumentAlignment"
#define LEUKO_RULE_NAME_ARRAY_ALIGNMENT                                "ArrayAlignment"
#define LEUKO_RULE_NAME_ASSIGNMENT_INDENTATION                         "AssignmentIndentation"
#define LEUKO_RULE_NAME_BEGIN_END_ALIGNMENT                            "BeginEndAlignment"
#define LEUKO_RULE_NAME_BLOCK_ALIGNMENT                                "BlockAlignment"
#define LEUKO_RULE_NAME_BLOCK_END_NEWLINE                              "BlockEndNewline"
#define LEUKO_RULE_NAME_CASE_INDENTATION                               "CaseIndentation"
#define LEUKO_RULE_NAME_CLASS_STRUCTURE                                "ClassStructure"
#define LEUKO_RULE_NAME_CLOSING_HEREDOC_INDENTATION                    "ClosingHeredocIndentation"
#define LEUKO_RULE_NAME_CLOSING_PARENTHESIS_INDENTATION                "ClosingParenthesisIndentation"
#define LEUKO_RULE_NAME_commonT_INDENTATION                            "commontIndentation"
#define LEUKO_RULE_NAME_CONDITION_POSITION                             "ConditionPosition"
#define LEUKO_RULE_NAME_DEF_END_ALIGNMENT                              "DefEndAlignment"
#define LEUKO_RULE_NAME_DOT_POSITION                                   "DotPosition"
#define LEUKO_RULE_NAME_ELSE_ALIGNMENT                                 "ElseAlignment"
#define LEUKO_RULE_NAME_EMPTY_commonT                                  "Emptycommont"
#define LEUKO_RULE_NAME_EMPTY_LINE_AFTER_GUARD_CLAUSE                  "EmptyLineAfterGuardClause"
#define LEUKO_RULE_NAME_EMPTY_LINE_AFTER_MAGIC_commonT                 "EmptyLineAfterMagiccommont"
#define LEUKO_RULE_NAME_EMPTY_LINE_AFTER_MULTILINE_CONDITION           "EmptyLineAfterMultilineCondition"
#define LEUKO_RULE_NAME_EMPTY_LINE_BETWEEN_DEFS                        "EmptyLineBetweenDefs"
#define LEUKO_RULE_NAME_EMPTY_LINES                                    "EmptyLines"
#define LEUKO_RULE_NAME_EMPTY_LINES_AFTER_MODULE_INCLUSION             "EmptyLinesAfterModuleInclusion"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_ACCESS_MODIFIER             "EmptyLinesAroundAccessModifier"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_ARGUMENTS                   "EmptyLinesAroundArguments"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_ATTRIBUTE_ACCESSOR          "EmptyLinesAroundAttributeAccessor"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_BEGIN_BODY                  "EmptyLinesAroundBeginBody"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_BLOCK_BODY                  "EmptyLinesAroundBlockBody"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_CLASS_BODY                  "EmptyLinesAroundClassBody"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_EXCEPTION_HANDLING_KEYWORDS "EmptyLinesAroundExceptionHandlingKeywords"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_METHOD_BODY                 "EmptyLinesAroundMethodBody"
#define LEUKO_RULE_NAME_EMPTY_LINES_AROUND_MODULE_BODY                 "EmptyLinesAroundModuleBody"
#define LEUKO_RULE_NAME_END_ALIGNMENT                                  "EndAlignment"
#define LEUKO_RULE_NAME_END_OF_LINE                                    "EndOfLine"
#define LEUKO_RULE_NAME_EXTRA_SPACING                                  "ExtraSpacing"
#define LEUKO_RULE_NAME_FIRST_ARGUMENT_INDENTATION                     "FirstArgumentIndentation"
#define LEUKO_RULE_NAME_FIRST_ARRAY_ELEMENT_INDENTATION                "FirstArrayElementIndentation"
#define LEUKO_RULE_NAME_FIRST_ARRAY_ELEMENT_LINE_BREAK                 "FirstArrayElementLineBreak"
#define LEUKO_RULE_NAME_FIRST_HASH_ELEMENT_INDENTATION                 "FirstHashElementIndentation"
#define LEUKO_RULE_NAME_FIRST_HASH_ELEMENT_LINE_BREAK                  "FirstHashElementLineBreak"
#define LEUKO_RULE_NAME_FIRST_METHOD_ARGUMENT_LINE_BREAK               "FirstMethodArgumentLineBreak"
#define LEUKO_RULE_NAME_FIRST_METHOD_PARAMETER_LINE_BREAK              "FirstMethodParameterLineBreak"
#define LEUKO_RULE_NAME_FIRST_PARAMETER_INDENTATION                    "FirstParameterIndentation"
#define LEUKO_RULE_NAME_HASH_ALIGNMENT                                 "HashAlignment"
#define LEUKO_RULE_NAME_HEREDOC_ARGUMENT_CLOSING_PARENTHESIS           "HeredocArgumentClosingParenthesis"
#define LEUKO_RULE_NAME_HEREDOC_INDENTATION                            "HeredocIndentation"
#define LEUKO_RULE_NAME_INDENTATION_CONSISTENCY                        "IndentationConsistency"
#define LEUKO_RULE_NAME_INDENTATION_STYLE                              "IndentationStyle"
#define LEUKO_RULE_NAME_INDENTATION_WIDTH                              "IndentationWidth"
#define LEUKO_RULE_NAME_INDENTATION_STYLE                              "IndentationStyle"
#define LEUKO_RULE_NAME_LINE_LENGTH                                    "LineLength"
#define LEUKO_RULE_NAME_INITIAL_INDENTATION                            "InitialIndentation"
#define LEUKO_RULE_NAME_LEADING_commonT_SPACE                          "LeadingcommontSpace"
#define LEUKO_RULE_NAME_LEADING_EMPTY_LINES                            "LeadingEmptyLines"
#define LEUKO_RULE_NAME_LINE_CONTINUATION_LEADING_SPACE                "LineContinuationLeadingSpace"
#define LEUKO_RULE_NAME_LINE_CONTINUATION_SPACING                      "LineContinuationSpacing"
#define LEUKO_RULE_NAME_LINE_END_STRING_CONCATENATION_INDENTATION      "LineEndStringConcatenationIndentation"
#define LEUKO_RULE_NAME_LINE_LENGTH                                    "LineLength"
#define LEUKO_RULE_NAME_MULTILINE_ARRAY_BRACE_LAYOUT                   "MultilineArrayBraceLayout"
#define LEUKO_RULE_NAME_MULTILINE_ARRAY_LINE_BREAKS                    "MultilineArrayLineBreaks"
#define LEUKO_RULE_NAME_MULTILINE_ASSIGNMENT_LAYOUT                    "MultilineAssignmentLayout"
#define LEUKO_RULE_NAME_MULTILINE_BLOCK_LAYOUT                         "MultilineBlockLayout"
#define LEUKO_RULE_NAME_MULTILINE_HASH_BRACE_LAYOUT                    "MultilineHashBraceLayout"
#define LEUKO_RULE_NAME_MULTILINE_HASH_KEY_LINE_BREAKS                 "MultilineHashKeyLineBreaks"
#define LEUKO_RULE_NAME_MULTILINE_METHOD_ARGUMENT_LINE_BREAKS          "MultilineMethodArgumentLineBreaks"
#define LEUKO_RULE_NAME_MULTILINE_METHOD_CALL_BRACE_LAYOUT             "MultilineMethodCallBraceLayout"
#define LEUKO_RULE_NAME_MULTILINE_METHOD_CALL_INDENTATION              "MultilineMethodCallIndentation"
#define LEUKO_RULE_NAME_MULTILINE_METHOD_DEFINITION_BRACE_LAYOUT       "MultilineMethodDefinitionBraceLayout"
#define LEUKO_RULE_NAME_MULTILINE_METHOD_PARAMETER_LINE_BREAKS         "MultilineMethodParameterLineBreaks"
#define LEUKO_RULE_NAME_MULTILINE_OPERATION_INDENTATION                "MultilineOperationIndentation"
#define LEUKO_RULE_NAME_PARAMETER_ALIGNMENT                            "ParameterAlignment"
#define LEUKO_RULE_NAME_REDUNDANT_LINE_BREAK                           "RedundantLineBreak"
#define LEUKO_RULE_NAME_RESCUE_ENSURE_ALIGNMENT                        "RescueEnsureAlignment"
#define LEUKO_RULE_NAME_SINGLE_LINE_BLOCK_CHAIN                        "SingleLineBlockChain"
#define LEUKO_RULE_NAME_SPACE_AFTER_COLON                              "SpaceAfterColon"
#define LEUKO_RULE_NAME_SPACE_AFTER_COMMA                              "SpaceAfterComma"
#define LEUKO_RULE_NAME_SPACE_AFTER_METHOD_NAME                        "SpaceAfterMethodName"
#define LEUKO_RULE_NAME_SPACE_AFTER_NOT                                "SpaceAfterNot"
#define LEUKO_RULE_NAME_SPACE_AFTER_SEMICOLON                          "SpaceAfterSemicolon"
#define LEUKO_RULE_NAME_SPACE_AROUND_BLOCK_PARAMETERS                  "SpaceAroundBlockParameters"
#define LEUKO_RULE_NAME_SPACE_AROUND_EQUALS_IN_PARAMETER_DEFAULT       "SpaceAroundEqualsInParameterDefault"
#define LEUKO_RULE_NAME_SPACE_AROUND_KEYWORD                           "SpaceAroundKeyword"
#define LEUKO_RULE_NAME_SPACE_AROUND_METHOD_CALL_OPERATOR              "SpaceAroundMethodCallOperator"
#define LEUKO_RULE_NAME_SPACE_AROUND_OPERATORS                         "SpaceAroundOperators"
#define LEUKO_RULE_NAME_SPACE_BEFORE_BLOCK_BRACES                      "SpaceBeforeBlockBraces"
#define LEUKO_RULE_NAME_SPACE_BEFORE_BRACKETS                          "SpaceBeforeBrackets"
#define LEUKO_RULE_NAME_SPACE_BEFORE_COMMA                             "SpaceBeforeComma"
#define LEUKO_RULE_NAME_SPACE_BEFORE_commonT                           "SpaceBeforecommont"
#define LEUKO_RULE_NAME_SPACE_BEFORE_FIRST_ARG                         "SpaceBeforeFirstArg"
#define LEUKO_RULE_NAME_SPACE_BEFORE_SEMICOLON                         "SpaceBeforeSemicolon"
#define LEUKO_RULE_NAME_SPACE_IN_LAMBDA_LITERAL                        "SpaceInLambdaLiteral"
#define LEUKO_RULE_NAME_SPACE_INSIDE_ARRAY_LITERAL_BRACKETS            "SpaceInsideArrayLiteralBrackets"
#define LEUKO_RULE_NAME_SPACE_INSIDE_ARRAY_PERCENT_LITERAL             "SpaceInsideArrayPercentLiteral"
#define LEUKO_RULE_NAME_SPACE_INSIDE_BLOCK_BRACES                      "SpaceInsideBlockBraces"
#define LEUKO_RULE_NAME_SPACE_INSIDE_HASH_LITERAL_BRACES               "SpaceInsideHashLiteralBraces"
#define LEUKO_RULE_NAME_SPACE_INSIDE_PARENS                            "SpaceInsideParens"
#define LEUKO_RULE_NAME_SPACE_INSIDE_PERCENT_LITERAL_DELIMITERS        "SpaceInsidePercentLiteralDelimiters"
#define LEUKO_RULE_NAME_SPACE_INSIDE_RANGE_LITERAL                     "SpaceInsideRangeLiteral"
#define LEUKO_RULE_NAME_SPACE_INSIDE_REFERENCE_BRACKETS                "SpaceInsideReferenceBrackets"
#define LEUKO_RULE_NAME_SPACE_INSIDE_STRING_INTERPOLATION              "SpaceInsideStringInterpolation"
#define LEUKO_RULE_NAME_TRAILING_EMPTY_LINES                           "TrailingEmptyLines"
#define LEUKO_RULE_NAME_TRAILING_WHITESPACE                            "TrailingWhitespace"
/* clang-format on */

/* Lint Rules */
/* clang-format off */
/* clang-format on */

#endif /* LEUKOCYTE_RULE_REGISTRY_H */
