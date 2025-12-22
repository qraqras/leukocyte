#ifndef LEUKOCYTE_RULE_REGISTRY_H
#define LEUKOCYTE_RULE_REGISTRY_H

#include <stddef.h>

#include "category.h"
#include "configs/rule_config.h"

/**
 * @brief Rule structure forward declaration.
 */
typedef struct rule_s rule_t;

/**
 * @brief Rule registry entry structure.
 */
typedef struct rule_registry_entry_s
{
    const char *category_name;                      /* e.g., "Layout" */
    const char *rule_name;                          /* e.g., "AccessModifierIndentation" */
    const char *full_name;                          /* e.g., "Layout/AccessModifierIndentation" */
    rule_t *rule;                                   /* Pointer to the rule instance */
    const struct leuko_rule_config_handlers_s *ops; /* Pointer to the config operations */
} rule_registry_entry_t;

const rule_registry_entry_t *leuko_get_rule_registry(void);
size_t leuko_get_rule_registry_count(void);

#define LEUKO_FULLNAME(cat, sname) cat "/" sname

/* Category Rules */
/* clang-format off */
#define LEUKO_LAYOUT "Layout"
#define LEUKO_LINT   "Lint"
/* clang-format on */

/* Layout Rules */
/* clang-format off */
#define LEUKO_SHORTNAME_ACCESS_MODIFIER_INDENTATION                    "AccessModifierIndentation"
#define LEUKO_SHORTNAME_ARGUMENT_ALIGNMENT                             "ArgumentAlignment"
#define LEUKO_SHORTNAME_ARRAY_ALIGNMENT                                "ArrayAlignment"
#define LEUKO_SHORTNAME_ASSIGNMENT_INDENTATION                         "AssignmentIndentation"
#define LEUKO_SHORTNAME_BEGIN_END_ALIGNMENT                            "BeginEndAlignment"
#define LEUKO_SHORTNAME_BLOCK_ALIGNMENT                                "BlockAlignment"
#define LEUKO_SHORTNAME_BLOCK_END_NEWLINE                              "BlockEndNewline"
#define LEUKO_SHORTNAME_CASE_INDENTATION                               "CaseIndentation"
#define LEUKO_SHORTNAME_CLASS_STRUCTURE                                "ClassStructure"
#define LEUKO_SHORTNAME_CLOSING_HEREDOC_INDENTATION                    "ClosingHeredocIndentation"
#define LEUKO_SHORTNAME_CLOSING_PARENTHESIS_INDENTATION                "ClosingParenthesisIndentation"
#define LEUKO_SHORTNAME_COMMENT_INDENTATION                            "CommentIndentation"
#define LEUKO_SHORTNAME_CONDITION_POSITION                             "ConditionPosition"
#define LEUKO_SHORTNAME_DEF_END_ALIGNMENT                              "DefEndAlignment"
#define LEUKO_SHORTNAME_DOT_POSITION                                   "DotPosition"
#define LEUKO_SHORTNAME_ELSE_ALIGNMENT                                 "ElseAlignment"
#define LEUKO_SHORTNAME_EMPTY_COMMENT                                  "EmptyComment"
#define LEUKO_SHORTNAME_EMPTY_LINE_AFTER_GUARD_CLAUSE                  "EmptyLineAfterGuardClause"
#define LEUKO_SHORTNAME_EMPTY_LINE_AFTER_MAGIC_COMMENT                 "EmptyLineAfterMagicComment"
#define LEUKO_SHORTNAME_EMPTY_LINE_AFTER_MULTILINE_CONDITION           "EmptyLineAfterMultilineCondition"
#define LEUKO_SHORTNAME_EMPTY_LINE_BETWEEN_DEFS                        "EmptyLineBetweenDefs"
#define LEUKO_SHORTNAME_EMPTY_LINES                                    "EmptyLines"
#define LEUKO_SHORTNAME_EMPTY_LINES_AFTER_MODULE_INCLUSION             "EmptyLinesAfterModuleInclusion"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_ACCESS_MODIFIER             "EmptyLinesAroundAccessModifier"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_ARGUMENTS                   "EmptyLinesAroundArguments"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_ATTRIBUTE_ACCESSOR          "EmptyLinesAroundAttributeAccessor"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_BEGIN_BODY                  "EmptyLinesAroundBeginBody"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_BLOCK_BODY                  "EmptyLinesAroundBlockBody"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_CLASS_BODY                  "EmptyLinesAroundClassBody"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_EXCEPTION_HANDLING_KEYWORDS "EmptyLinesAroundExceptionHandlingKeywords"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_METHOD_BODY                 "EmptyLinesAroundMethodBody"
#define LEUKO_SHORTNAME_EMPTY_LINES_AROUND_MODULE_BODY                 "EmptyLinesAroundModuleBody"
#define LEUKO_SHORTNAME_END_ALIGNMENT                                  "EndAlignment"
#define LEUKO_SHORTNAME_END_OF_LINE                                    "EndOfLine"
#define LEUKO_SHORTNAME_EXTRA_SPACING                                  "ExtraSpacing"
#define LEUKO_SHORTNAME_FIRST_ARGUMENT_INDENTATION                     "FirstArgumentIndentation"
#define LEUKO_SHORTNAME_FIRST_ARRAY_ELEMENT_INDENTATION                "FirstArrayElementIndentation"
#define LEUKO_SHORTNAME_FIRST_ARRAY_ELEMENT_LINE_BREAK                 "FirstArrayElementLineBreak"
#define LEUKO_SHORTNAME_FIRST_HASH_ELEMENT_INDENTATION                 "FirstHashElementIndentation"
#define LEUKO_SHORTNAME_FIRST_HASH_ELEMENT_LINE_BREAK                  "FirstHashElementLineBreak"
#define LEUKO_SHORTNAME_FIRST_METHOD_ARGUMENT_LINE_BREAK               "FirstMethodArgumentLineBreak"
#define LEUKO_SHORTNAME_FIRST_METHOD_PARAMETER_LINE_BREAK              "FirstMethodParameterLineBreak"
#define LEUKO_SHORTNAME_FIRST_PARAMETER_INDENTATION                    "FirstParameterIndentation"
#define LEUKO_SHORTNAME_HASH_ALIGNMENT                                 "HashAlignment"
#define LEUKO_SHORTNAME_HEREDOC_ARGUMENT_CLOSING_PARENTHESIS           "HeredocArgumentClosingParenthesis"
#define LEUKO_SHORTNAME_HEREDOC_INDENTATION                            "HeredocIndentation"
#define LEUKO_SHORTNAME_INDENTATION_CONSISTENCY                        "IndentationConsistency"
#define LEUKO_SHORTNAME_INDENTATION_STYLE                              "IndentationStyle"
#define LEUKO_SHORTNAME_INDENTATION_WIDTH                              "IndentationWidth"
#define LEUKO_SHORTNAME_INITIAL_INDENTATION                            "InitialIndentation"
#define LEUKO_SHORTNAME_LEADING_COMMENT_SPACE                          "LeadingCommentSpace"
#define LEUKO_SHORTNAME_LEADING_EMPTY_LINES                            "LeadingEmptyLines"
#define LEUKO_SHORTNAME_LINE_CONTINUATION_LEADING_SPACE                "LineContinuationLeadingSpace"
#define LEUKO_SHORTNAME_LINE_CONTINUATION_SPACING                      "LineContinuationSpacing"
#define LEUKO_SHORTNAME_LINE_END_STRING_CONCATENATION_INDENTATION      "LineEndStringConcatenationIndentation"
#define LEUKO_SHORTNAME_LINE_LENGTH                                    "LineLength"
#define LEUKO_SHORTNAME_MULTILINE_ARRAY_BRACE_LAYOUT                   "MultilineArrayBraceLayout"
#define LEUKO_SHORTNAME_MULTILINE_ARRAY_LINE_BREAKS                    "MultilineArrayLineBreaks"
#define LEUKO_SHORTNAME_MULTILINE_ASSIGNMENT_LAYOUT                    "MultilineAssignmentLayout"
#define LEUKO_SHORTNAME_MULTILINE_BLOCK_LAYOUT                         "MultilineBlockLayout"
#define LEUKO_SHORTNAME_MULTILINE_HASH_BRACE_LAYOUT                    "MultilineHashBraceLayout"
#define LEUKO_SHORTNAME_MULTILINE_HASH_KEY_LINE_BREAKS                 "MultilineHashKeyLineBreaks"
#define LEUKO_SHORTNAME_MULTILINE_METHOD_ARGUMENT_LINE_BREAKS          "MultilineMethodArgumentLineBreaks"
#define LEUKO_SHORTNAME_MULTILINE_METHOD_CALL_BRACE_LAYOUT             "MultilineMethodCallBraceLayout"
#define LEUKO_SHORTNAME_MULTILINE_METHOD_CALL_INDENTATION              "MultilineMethodCallIndentation"
#define LEUKO_SHORTNAME_MULTILINE_METHOD_DEFINITION_BRACE_LAYOUT       "MultilineMethodDefinitionBraceLayout"
#define LEUKO_SHORTNAME_MULTILINE_METHOD_PARAMETER_LINE_BREAKS         "MultilineMethodParameterLineBreaks"
#define LEUKO_SHORTNAME_MULTILINE_OPERATION_INDENTATION                "MultilineOperationIndentation"
#define LEUKO_SHORTNAME_PARAMETER_ALIGNMENT                            "ParameterAlignment"
#define LEUKO_SHORTNAME_REDUNDANT_LINE_BREAK                           "RedundantLineBreak"
#define LEUKO_SHORTNAME_RESCUE_ENSURE_ALIGNMENT                        "RescueEnsureAlignment"
#define LEUKO_SHORTNAME_SINGLE_LINE_BLOCK_CHAIN                        "SingleLineBlockChain"
#define LEUKO_SHORTNAME_SPACE_AFTER_COLON                              "SpaceAfterColon"
#define LEUKO_SHORTNAME_SPACE_AFTER_COMMA                              "SpaceAfterComma"
#define LEUKO_SHORTNAME_SPACE_AFTER_METHOD_NAME                        "SpaceAfterMethodName"
#define LEUKO_SHORTNAME_SPACE_AFTER_NOT                                "SpaceAfterNot"
#define LEUKO_SHORTNAME_SPACE_AFTER_SEMICOLON                          "SpaceAfterSemicolon"
#define LEUKO_SHORTNAME_SPACE_AROUND_BLOCK_PARAMETERS                  "SpaceAroundBlockParameters"
#define LEUKO_SHORTNAME_SPACE_AROUND_EQUALS_IN_PARAMETER_DEFAULT       "SpaceAroundEqualsInParameterDefault"
#define LEUKO_SHORTNAME_SPACE_AROUND_KEYWORD                           "SpaceAroundKeyword"
#define LEUKO_SHORTNAME_SPACE_AROUND_METHOD_CALL_OPERATOR              "SpaceAroundMethodCallOperator"
#define LEUKO_SHORTNAME_SPACE_AROUND_OPERATORS                         "SpaceAroundOperators"
#define LEUKO_SHORTNAME_SPACE_BEFORE_BLOCK_BRACES                      "SpaceBeforeBlockBraces"
#define LEUKO_SHORTNAME_SPACE_BEFORE_BRACKETS                          "SpaceBeforeBrackets"
#define LEUKO_SHORTNAME_SPACE_BEFORE_COMMA                             "SpaceBeforeComma"
#define LEUKO_SHORTNAME_SPACE_BEFORE_COMMENT                           "SpaceBeforeComment"
#define LEUKO_SHORTNAME_SPACE_BEFORE_FIRST_ARG                         "SpaceBeforeFirstArg"
#define LEUKO_SHORTNAME_SPACE_BEFORE_SEMICOLON                         "SpaceBeforeSemicolon"
#define LEUKO_SHORTNAME_SPACE_IN_LAMBDA_LITERAL                        "SpaceInLambdaLiteral"
#define LEUKO_SHORTNAME_SPACE_INSIDE_ARRAY_LITERAL_BRACKETS            "SpaceInsideArrayLiteralBrackets"
#define LEUKO_SHORTNAME_SPACE_INSIDE_ARRAY_PERCENT_LITERAL             "SpaceInsideArrayPercentLiteral"
#define LEUKO_SHORTNAME_SPACE_INSIDE_BLOCK_BRACES                      "SpaceInsideBlockBraces"
#define LEUKO_SHORTNAME_SPACE_INSIDE_HASH_LITERAL_BRACES               "SpaceInsideHashLiteralBraces"
#define LEUKO_SHORTNAME_SPACE_INSIDE_PARENS                            "SpaceInsideParens"
#define LEUKO_SHORTNAME_SPACE_INSIDE_PERCENT_LITERAL_DELIMITERS        "SpaceInsidePercentLiteralDelimiters"
#define LEUKO_SHORTNAME_SPACE_INSIDE_RANGE_LITERAL                     "SpaceInsideRangeLiteral"
#define LEUKO_SHORTNAME_SPACE_INSIDE_REFERENCE_BRACKETS                "SpaceInsideReferenceBrackets"
#define LEUKO_SHORTNAME_SPACE_INSIDE_STRING_INTERPOLATION              "SpaceInsideStringInterpolation"
#define LEUKO_SHORTNAME_TRAILING_EMPTY_LINES                           "TrailingEmptyLines"
#define LEUKO_SHORTNAME_TRAILING_WHITESPACE                            "TrailingWhitespace"
/* clang-format on */

/* Lint Rules */
/* clang-format off */
/* clang-format on */

#endif /* LEUKOCYTE_RULE_REGISTRY_H */
