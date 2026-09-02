#include "testing.h"
#include "lexer.h"

DECLARE_TEST(lexer)
{
    init_lexer();

    const char *text = "int main() {}";

    token_t tok;
    int i = 0;
    while (lex(text, &tok))
    {
        switch (i)
        {
        case 0:
            TEST_ASSERT_STR(tok.text, "int");
            TEST_ASSERT_AB(tok.type, TOKTYPE_KEYWORD);
            break;

        case 1:
            TEST_ASSERT_STR(tok.text, "main");
            TEST_ASSERT_AB(tok.type, TOKTYPE_IDENTIFIER);
            break;

        case 2:
            TEST_ASSERT_STR(tok.text, "(");
            TEST_ASSERT_AB(tok.type, TOKTYPE_LPAREN);
            break;

        case 3:
            TEST_ASSERT_STR(tok.text, ")");
            TEST_ASSERT_AB(tok.type, TOKTYPE_RPAREN);
            break;

        case 4:
            TEST_ASSERT_STR(tok.text, "{");
            TEST_ASSERT_AB(tok.type, TOKTYPE_LBRACE);
            break;

        case 5:
            TEST_ASSERT_STR(tok.text, "}");
            TEST_ASSERT_AB(tok.type, TOKTYPE_RBRACE);
            break;

        default:
            TEST_FAIL("lexer generated more than 6 token");
        }
        i++;
    }

    TEST_SUCCESS();
}
