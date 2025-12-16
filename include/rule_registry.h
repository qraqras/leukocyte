#ifndef LEUKOCYTE_RULES_RULE_REGISTRY_H
#define LEUKOCYTE_RULES_RULE_REGISTRY_H

#include <stddef.h>

#include "category.h"
#include "configs/config_ops.h"

/* Forward declare rule_t to avoid circular includes with rules/rule.h */
typedef struct rule_s rule_t;

/// @brief Rule registry entry.
typedef struct rule_registry_entry_s
{
    category_t category;
    const char *rule_name;
    rule_t *rule;
    const struct config_ops *ops;
} rule_registry_entry_t;

const rule_registry_entry_t *get_rule_registry(void);
size_t get_rule_registry_count(void);

/* Category Rules */
/* clang-format off */
#define LAYOUT "Layout"
#define LINT   "Lint"
/* clang-format on */

/* Layout Rules */
/* clang-format off */
#define LAYOUT_ACCESS_MODIFIER_INDENTATION                    "Layout/AccessModifierIndentation"
#define LAYOUT_ARGUMENT_ALIGNMENT                             "Layout/ArgumentAlignment"
#define LAYOUT_ARRAY_ALIGNMENT                                "Layout/ArrayAlignment"
#define LAYOUT_ASSIGNMENT_INDENTATION                         "Layout/AssignmentIndentation"
#define LAYOUT_BEGIN_END_ALIGNMENT                            "Layout/BeginEndAlignment"
#define LAYOUT_BLOCK_ALIGNMENT                                "Layout/BlockAlignment"
#define LAYOUT_BLOCK_END_NEWLINE                              "Layout/BlockEndNewline"
#define LAYOUT_CASE_INDENTATION                               "Layout/CaseIndentation"
#define LAYOUT_CLASS_STRUCTURE                                "Layout/ClassStructure"
#define LAYOUT_CLOSING_HEREDOC_INDENTATION                    "Layout/ClosingHeredocIndentation"
#define LAYOUT_CLOSING_PARENTHESIS_INDENTATION                "Layout/ClosingParenthesisIndentation"
#define LAYOUT_COMMENT_INDENTATION                            "Layout/CommentIndentation"
#define LAYOUT_CONDITION_POSITION                             "Layout/ConditionPosition"
#define LAYOUT_DEF_END_ALIGNMENT                              "Layout/DefEndAlignment"
#define LAYOUT_DOT_POSITION                                   "Layout/DotPosition"
#define LAYOUT_ELSE_ALIGNMENT                                 "Layout/ElseAlignment"
#define LAYOUT_EMPTY_COMMENT                                  "Layout/EmptyComment"
#define LAYOUT_EMPTY_LINE_AFTER_GUARD_CLAUSE                  "Layout/EmptyLineAfterGuardClause"
#define LAYOUT_EMPTY_LINE_AFTER_MAGIC_COMMENT                 "Layout/EmptyLineAfterMagicComment"
#define LAYOUT_EMPTY_LINE_AFTER_MULTILINE_CONDITION           "Layout/EmptyLineAfterMultilineCondition"
#define LAYOUT_EMPTY_LINE_BETWEEN_DEFS                        "Layout/EmptyLineBetweenDefs"
#define LAYOUT_EMPTY_LINES                                    "Layout/EmptyLines"
#define LAYOUT_EMPTY_LINES_AFTER_MODULE_INCLUSION             "Layout/EmptyLinesAfterModuleInclusion"
#define LAYOUT_EMPTY_LINES_AROUND_ACCESS_MODIFIER             "Layout/EmptyLinesAroundAccessModifier"
#define LAYOUT_EMPTY_LINES_AROUND_ARGUMENTS                   "Layout/EmptyLinesAroundArguments"
#define LAYOUT_EMPTY_LINES_AROUND_ATTRIBUTE_ACCESSOR          "Layout/EmptyLinesAroundAttributeAccessor"
#define LAYOUT_EMPTY_LINES_AROUND_BEGIN_BODY                  "Layout/EmptyLinesAroundBeginBody"
#define LAYOUT_EMPTY_LINES_AROUND_BLOCK_BODY                  "Layout/EmptyLinesAroundBlockBody"
#define LAYOUT_EMPTY_LINES_AROUND_CLASS_BODY                  "Layout/EmptyLinesAroundClassBody"
#define LAYOUT_EMPTY_LINES_AROUND_EXCEPTION_HANDLING_KEYWORDS "Layout/EmptyLinesAroundExceptionHandlingKeywords"
#define LAYOUT_EMPTY_LINES_AROUND_METHOD_BODY                 "Layout/EmptyLinesAroundMethodBody"
#define LAYOUT_EMPTY_LINES_AROUND_MODULE_BODY                 "Layout/EmptyLinesAroundModuleBody"
#define LAYOUT_END_ALIGNMENT                                  "Layout/EndAlignment"
#define LAYOUT_END_OF_LINE                                    "Layout/EndOfLine"
#define LAYOUT_EXTRA_SPACING                                  "Layout/ExtraSpacing"
#define LAYOUT_FIRST_ARGUMENT_INDENTATION                     "Layout/FirstArgumentIndentation"
#define LAYOUT_FIRST_ARRAY_ELEMENT_INDENTATION                "Layout/FirstArrayElementIndentation"
#define LAYOUT_FIRST_ARRAY_ELEMENT_LINE_BREAK                 "Layout/FirstArrayElementLineBreak"
#define LAYOUT_FIRST_HASH_ELEMENT_INDENTATION                 "Layout/FirstHashElementIndentation"
#define LAYOUT_FIRST_HASH_ELEMENT_LINE_BREAK                  "Layout/FirstHashElementLineBreak"
#define LAYOUT_FIRST_METHOD_ARGUMENT_LINE_BREAK               "Layout/FirstMethodArgumentLineBreak"
#define LAYOUT_FIRST_METHOD_PARAMETER_LINE_BREAK              "Layout/FirstMethodParameterLineBreak"
#define LAYOUT_FIRST_PARAMETER_INDENTATION                    "Layout/FirstParameterIndentation"
#define LAYOUT_HASH_ALIGNMENT                                 "Layout/HashAlignment"
#define LAYOUT_HEREDOC_ARGUMENT_CLOSING_PARENTHESIS           "Layout/HeredocArgumentClosingParenthesis"
#define LAYOUT_HEREDOC_INDENTATION                            "Layout/HeredocIndentation"
#define LAYOUT_INDENTATION_CONSISTENCY                        "Layout/IndentationConsistency"
#define LAYOUT_INDENTATION_STYLE                              "Layout/IndentationStyle"
#define LAYOUT_INDENTATION_WIDTH                              "Layout/IndentationWidth"
#define LAYOUT_INITIAL_INDENTATION                            "Layout/InitialIndentation"
#define LAYOUT_LEADING_COMMENT_SPACE                          "Layout/LeadingCommentSpace"
#define LAYOUT_LEADING_EMPTY_LINES                            "Layout/LeadingEmptyLines"
#define LAYOUT_LINE_CONTINUATION_LEADING_SPACE                "Layout/LineContinuationLeadingSpace"
#define LAYOUT_LINE_CONTINUATION_SPACING                      "Layout/LineContinuationSpacing"
#define LAYOUT_LINE_END_STRING_CONCATENATION_INDENTATION      "Layout/LineEndStringConcatenationIndentation"
#define LAYOUT_LINE_LENGTH                                    "Layout/LineLength"
#define LAYOUT_MULTILINE_ARRAY_BRACE_LAYOUT                   "Layout/MultilineArrayBraceLayout"
#define LAYOUT_MULTILINE_ARRAY_LINE_BREAKS                    "Layout/MultilineArrayLineBreaks"
#define LAYOUT_MULTILINE_ASSIGNMENT_LAYOUT                    "Layout/MultilineAssignmentLayout"
#define LAYOUT_MULTILINE_BLOCK_LAYOUT                         "Layout/MultilineBlockLayout"
#define LAYOUT_MULTILINE_HASH_BRACE_LAYOUT                    "Layout/MultilineHashBraceLayout"
#define LAYOUT_MULTILINE_HASH_KEY_LINE_BREAKS                 "Layout/MultilineHashKeyLineBreaks"
#define LAYOUT_MULTILINE_METHOD_ARGUMENT_LINE_BREAKS          "Layout/MultilineMethodArgumentLineBreaks"
#define LAYOUT_MULTILINE_METHOD_CALL_BRACE_LAYOUT             "Layout/MultilineMethodCallBraceLayout"
#define LAYOUT_MULTILINE_METHOD_CALL_INDENTATION              "Layout/MultilineMethodCallIndentation"
#define LAYOUT_MULTILINE_METHOD_DEFINITION_BRACE_LAYOUT       "Layout/MultilineMethodDefinitionBraceLayout"
#define LAYOUT_MULTILINE_METHOD_PARAMETER_LINE_BREAKS         "Layout/MultilineMethodParameterLineBreaks"
#define LAYOUT_MULTILINE_OPERATION_INDENTATION                "Layout/MultilineOperationIndentation"
#define LAYOUT_PARAMETER_ALIGNMENT                            "Layout/ParameterAlignment"
#define LAYOUT_REDUNDANT_LINE_BREAK                           "Layout/RedundantLineBreak"
#define LAYOUT_RESCUE_ENSURE_ALIGNMENT                        "Layout/RescueEnsureAlignment"
#define LAYOUT_SINGLE_LINE_BLOCK_CHAIN                        "Layout/SingleLineBlockChain"
#define LAYOUT_SPACE_AFTER_COLON                              "Layout/SpaceAfterColon"
#define LAYOUT_SPACE_AFTER_COMMA                              "Layout/SpaceAfterComma"
#define LAYOUT_SPACE_AFTER_METHOD_NAME                        "Layout/SpaceAfterMethodName"
#define LAYOUT_SPACE_AFTER_NOT                                "Layout/SpaceAfterNot"
#define LAYOUT_SPACE_AFTER_SEMICOLON                          "Layout/SpaceAfterSemicolon"
#define LAYOUT_SPACE_AROUND_BLOCK_PARAMETERS                  "Layout/SpaceAroundBlockParameters"
#define LAYOUT_SPACE_AROUND_EQUALS_IN_PARAMETER_DEFAULT       "Layout/SpaceAroundEqualsInParameterDefault"
#define LAYOUT_SPACE_AROUND_KEYWORD                           "Layout/SpaceAroundKeyword"
#define LAYOUT_SPACE_AROUND_METHOD_CALL_OPERATOR              "Layout/SpaceAroundMethodCallOperator"
#define LAYOUT_SPACE_AROUND_OPERATORS                         "Layout/SpaceAroundOperators"
#define LAYOUT_SPACE_BEFORE_BLOCK_BRACES                      "Layout/SpaceBeforeBlockBraces"
#define LAYOUT_SPACE_BEFORE_BRACKETS                          "Layout/SpaceBeforeBrackets"
#define LAYOUT_SPACE_BEFORE_COMMA                             "Layout/SpaceBeforeComma"
#define LAYOUT_SPACE_BEFORE_COMMENT                           "Layout/SpaceBeforeComment"
#define LAYOUT_SPACE_BEFORE_FIRST_ARG                         "Layout/SpaceBeforeFirstArg"
#define LAYOUT_SPACE_BEFORE_SEMICOLON                         "Layout/SpaceBeforeSemicolon"
#define LAYOUT_SPACE_IN_LAMBDA_LITERAL                        "Layout/SpaceInLambdaLiteral"
#define LAYOUT_SPACE_INSIDE_ARRAY_LITERAL_BRACKETS            "Layout/SpaceInsideArrayLiteralBrackets"
#define LAYOUT_SPACE_INSIDE_ARRAY_PERCENT_LITERAL             "Layout/SpaceInsideArrayPercentLiteral"
#define LAYOUT_SPACE_INSIDE_BLOCK_BRACES                      "Layout/SpaceInsideBlockBraces"
#define LAYOUT_SPACE_INSIDE_HASH_LITERAL_BRACES               "Layout/SpaceInsideHashLiteralBraces"
#define LAYOUT_SPACE_INSIDE_PARENS                            "Layout/SpaceInsideParens"
#define LAYOUT_SPACE_INSIDE_PERCENT_LITERAL_DELIMITERS        "Layout/SpaceInsidePercentLiteralDelimiters"
#define LAYOUT_SPACE_INSIDE_RANGE_LITERAL                     "Layout/SpaceInsideRangeLiteral"
#define LAYOUT_SPACE_INSIDE_REFERENCE_BRACKETS                "Layout/SpaceInsideReferenceBrackets"
#define LAYOUT_SPACE_INSIDE_STRING_INTERPOLATION              "Layout/SpaceInsideStringInterpolation"
#define LAYOUT_TRAILING_EMPTY_LINES                           "Layout/TrailingEmptyLines"
#define LAYOUT_TRAILING_WHITESPACE                            "Layout/TrailingWhitespace"
/* clang-format on */

/* Lint Rules */
/* clang-format off */
/* clang-format on */

#endif /* LEUKOCYTE_RULES_RULE_REGISTRY_H */
