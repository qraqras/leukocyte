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
    const char *category_name;    /* e.g., "Layout" */
    const char *rule_name;        /* e.g., "AccessModifierIndentation" */
    const char *full_name;        /* e.g., "Layout/AccessModifierIndentation" */
    rule_t *rule;                 /* Pointer to the rule instance */
    const struct config_ops *ops; /* Pointer to the config operations */
} rule_registry_entry_t;

const rule_registry_entry_t *get_rule_registry(void);
size_t get_rule_registry_count(void);

#define FULLNAME(cat, sname) cat "/" sname

/* Category Rules */
/* clang-format off */
#define LAYOUT "Layout"
#define LINT   "Lint"
/* clang-format on */

/* Layout Rules */
/* clang-format off */
#define SHORTNAME_ACCESS_MODIFIER_INDENTATION                    "AccessModifierIndentation"
#define SHORTNAME_ARGUMENT_ALIGNMENT                             "ArgumentAlignment"
#define SHORTNAME_ARRAY_ALIGNMENT                                "ArrayAlignment"
#define SHORTNAME_ASSIGNMENT_INDENTATION                         "AssignmentIndentation"
#define SHORTNAME_BEGIN_END_ALIGNMENT                            "BeginEndAlignment"
#define SHORTNAME_BLOCK_ALIGNMENT                                "BlockAlignment"
#define SHORTNAME_BLOCK_END_NEWLINE                              "BlockEndNewline"
#define SHORTNAME_CASE_INDENTATION                               "CaseIndentation"
#define SHORTNAME_CLASS_STRUCTURE                                "ClassStructure"
#define SHORTNAME_CLOSING_HEREDOC_INDENTATION                    "ClosingHeredocIndentation"
#define SHORTNAME_CLOSING_PARENTHESIS_INDENTATION                "ClosingParenthesisIndentation"
#define SHORTNAME_COMMENT_INDENTATION                            "CommentIndentation"
#define SHORTNAME_CONDITION_POSITION                             "ConditionPosition"
#define SHORTNAME_DEF_END_ALIGNMENT                              "DefEndAlignment"
#define SHORTNAME_DOT_POSITION                                   "DotPosition"
#define SHORTNAME_ELSE_ALIGNMENT                                 "ElseAlignment"
#define SHORTNAME_EMPTY_COMMENT                                  "EmptyComment"
#define SHORTNAME_EMPTY_LINE_AFTER_GUARD_CLAUSE                  "EmptyLineAfterGuardClause"
#define SHORTNAME_EMPTY_LINE_AFTER_MAGIC_COMMENT                 "EmptyLineAfterMagicComment"
#define SHORTNAME_EMPTY_LINE_AFTER_MULTILINE_CONDITION           "EmptyLineAfterMultilineCondition"
#define SHORTNAME_EMPTY_LINE_BETWEEN_DEFS                        "EmptyLineBetweenDefs"
#define SHORTNAME_EMPTY_LINES                                    "EmptyLines"
#define SHORTNAME_EMPTY_LINES_AFTER_MODULE_INCLUSION             "EmptyLinesAfterModuleInclusion"
#define SHORTNAME_EMPTY_LINES_AROUND_ACCESS_MODIFIER             "EmptyLinesAroundAccessModifier"
#define SHORTNAME_EMPTY_LINES_AROUND_ARGUMENTS                   "EmptyLinesAroundArguments"
#define SHORTNAME_EMPTY_LINES_AROUND_ATTRIBUTE_ACCESSOR          "EmptyLinesAroundAttributeAccessor"
#define SHORTNAME_EMPTY_LINES_AROUND_BEGIN_BODY                  "EmptyLinesAroundBeginBody"
#define SHORTNAME_EMPTY_LINES_AROUND_BLOCK_BODY                  "EmptyLinesAroundBlockBody"
#define SHORTNAME_EMPTY_LINES_AROUND_CLASS_BODY                  "EmptyLinesAroundClassBody"
#define SHORTNAME_EMPTY_LINES_AROUND_EXCEPTION_HANDLING_KEYWORDS "EmptyLinesAroundExceptionHandlingKeywords"
#define SHORTNAME_EMPTY_LINES_AROUND_METHOD_BODY                 "EmptyLinesAroundMethodBody"
#define SHORTNAME_EMPTY_LINES_AROUND_MODULE_BODY                 "EmptyLinesAroundModuleBody"
#define SHORTNAME_END_ALIGNMENT                                  "EndAlignment"
#define SHORTNAME_END_OF_LINE                                    "EndOfLine"
#define SHORTNAME_EXTRA_SPACING                                  "ExtraSpacing"
#define SHORTNAME_FIRST_ARGUMENT_INDENTATION                     "FirstArgumentIndentation"
#define SHORTNAME_FIRST_ARRAY_ELEMENT_INDENTATION                "FirstArrayElementIndentation"
#define SHORTNAME_FIRST_ARRAY_ELEMENT_LINE_BREAK                 "FirstArrayElementLineBreak"
#define SHORTNAME_FIRST_HASH_ELEMENT_INDENTATION                 "FirstHashElementIndentation"
#define SHORTNAME_FIRST_HASH_ELEMENT_LINE_BREAK                  "FirstHashElementLineBreak"
#define SHORTNAME_FIRST_METHOD_ARGUMENT_LINE_BREAK               "FirstMethodArgumentLineBreak"
#define SHORTNAME_FIRST_METHOD_PARAMETER_LINE_BREAK              "FirstMethodParameterLineBreak"
#define SHORTNAME_FIRST_PARAMETER_INDENTATION                    "FirstParameterIndentation"
#define SHORTNAME_HASH_ALIGNMENT                                 "HashAlignment"
#define SHORTNAME_HEREDOC_ARGUMENT_CLOSING_PARENTHESIS           "HeredocArgumentClosingParenthesis"
#define SHORTNAME_HEREDOC_INDENTATION                            "HeredocIndentation"
#define SHORTNAME_INDENTATION_CONSISTENCY                        "IndentationConsistency"
#define SHORTNAME_INDENTATION_STYLE                              "IndentationStyle"
#define SHORTNAME_INDENTATION_WIDTH                              "IndentationWidth"
#define SHORTNAME_INITIAL_INDENTATION                            "InitialIndentation"
#define SHORTNAME_LEADING_COMMENT_SPACE                          "LeadingCommentSpace"
#define SHORTNAME_LEADING_EMPTY_LINES                            "LeadingEmptyLines"
#define SHORTNAME_LINE_CONTINUATION_LEADING_SPACE                "LineContinuationLeadingSpace"
#define SHORTNAME_LINE_CONTINUATION_SPACING                      "LineContinuationSpacing"
#define SHORTNAME_LINE_END_STRING_CONCATENATION_INDENTATION      "LineEndStringConcatenationIndentation"
#define SHORTNAME_LINE_LENGTH                                    "LineLength"
#define SHORTNAME_MULTILINE_ARRAY_BRACE_LAYOUT                   "MultilineArrayBraceLayout"
#define SHORTNAME_MULTILINE_ARRAY_LINE_BREAKS                    "MultilineArrayLineBreaks"
#define SHORTNAME_MULTILINE_ASSIGNMENT_LAYOUT                    "MultilineAssignmentLayout"
#define SHORTNAME_MULTILINE_BLOCK_LAYOUT                         "MultilineBlockLayout"
#define SHORTNAME_MULTILINE_HASH_BRACE_LAYOUT                    "MultilineHashBraceLayout"
#define SHORTNAME_MULTILINE_HASH_KEY_LINE_BREAKS                 "MultilineHashKeyLineBreaks"
#define SHORTNAME_MULTILINE_METHOD_ARGUMENT_LINE_BREAKS          "MultilineMethodArgumentLineBreaks"
#define SHORTNAME_MULTILINE_METHOD_CALL_BRACE_LAYOUT             "MultilineMethodCallBraceLayout"
#define SHORTNAME_MULTILINE_METHOD_CALL_INDENTATION              "MultilineMethodCallIndentation"
#define SHORTNAME_MULTILINE_METHOD_DEFINITION_BRACE_LAYOUT       "MultilineMethodDefinitionBraceLayout"
#define SHORTNAME_MULTILINE_METHOD_PARAMETER_LINE_BREAKS         "MultilineMethodParameterLineBreaks"
#define SHORTNAME_MULTILINE_OPERATION_INDENTATION                "MultilineOperationIndentation"
#define SHORTNAME_PARAMETER_ALIGNMENT                            "ParameterAlignment"
#define SHORTNAME_REDUNDANT_LINE_BREAK                           "RedundantLineBreak"
#define SHORTNAME_RESCUE_ENSURE_ALIGNMENT                        "RescueEnsureAlignment"
#define SHORTNAME_SINGLE_LINE_BLOCK_CHAIN                        "SingleLineBlockChain"
#define SHORTNAME_SPACE_AFTER_COLON                              "SpaceAfterColon"
#define SHORTNAME_SPACE_AFTER_COMMA                              "SpaceAfterComma"
#define SHORTNAME_SPACE_AFTER_METHOD_NAME                        "SpaceAfterMethodName"
#define SHORTNAME_SPACE_AFTER_NOT                                "SpaceAfterNot"
#define SHORTNAME_SPACE_AFTER_SEMICOLON                          "SpaceAfterSemicolon"
#define SHORTNAME_SPACE_AROUND_BLOCK_PARAMETERS                  "SpaceAroundBlockParameters"
#define SHORTNAME_SPACE_AROUND_EQUALS_IN_PARAMETER_DEFAULT       "SpaceAroundEqualsInParameterDefault"
#define SHORTNAME_SPACE_AROUND_KEYWORD                           "SpaceAroundKeyword"
#define SHORTNAME_SPACE_AROUND_METHOD_CALL_OPERATOR              "SpaceAroundMethodCallOperator"
#define SHORTNAME_SPACE_AROUND_OPERATORS                         "SpaceAroundOperators"
#define SHORTNAME_SPACE_BEFORE_BLOCK_BRACES                      "SpaceBeforeBlockBraces"
#define SHORTNAME_SPACE_BEFORE_BRACKETS                          "SpaceBeforeBrackets"
#define SHORTNAME_SPACE_BEFORE_COMMA                             "SpaceBeforeComma"
#define SHORTNAME_SPACE_BEFORE_COMMENT                           "SpaceBeforeComment"
#define SHORTNAME_SPACE_BEFORE_FIRST_ARG                         "SpaceBeforeFirstArg"
#define SHORTNAME_SPACE_BEFORE_SEMICOLON                         "SpaceBeforeSemicolon"
#define SHORTNAME_SPACE_IN_LAMBDA_LITERAL                        "SpaceInLambdaLiteral"
#define SHORTNAME_SPACE_INSIDE_ARRAY_LITERAL_BRACKETS            "SpaceInsideArrayLiteralBrackets"
#define SHORTNAME_SPACE_INSIDE_ARRAY_PERCENT_LITERAL             "SpaceInsideArrayPercentLiteral"
#define SHORTNAME_SPACE_INSIDE_BLOCK_BRACES                      "SpaceInsideBlockBraces"
#define SHORTNAME_SPACE_INSIDE_HASH_LITERAL_BRACES               "SpaceInsideHashLiteralBraces"
#define SHORTNAME_SPACE_INSIDE_PARENS                            "SpaceInsideParens"
#define SHORTNAME_SPACE_INSIDE_PERCENT_LITERAL_DELIMITERS        "SpaceInsidePercentLiteralDelimiters"
#define SHORTNAME_SPACE_INSIDE_RANGE_LITERAL                     "SpaceInsideRangeLiteral"
#define SHORTNAME_SPACE_INSIDE_REFERENCE_BRACKETS                "SpaceInsideReferenceBrackets"
#define SHORTNAME_SPACE_INSIDE_STRING_INTERPOLATION              "SpaceInsideStringInterpolation"
#define SHORTNAME_TRAILING_EMPTY_LINES                           "TrailingEmptyLines"
#define SHORTNAME_TRAILING_WHITESPACE                            "TrailingWhitespace"
/* clang-format on */

/* Lint Rules */
/* clang-format off */
/* clang-format on */

#endif /* LEUKOCYTE_RULES_RULE_REGISTRY_H */
