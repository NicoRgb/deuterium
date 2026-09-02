#include "testing.h"
#include "tokstream.h"

DECLARE_TEST(tokstream)
{
    init_lexer();

    const char *text = "int main() {}";
    tokstream_init(text);

    TEST_ASSERT_A(tok_expect_kw("int"));
    TEST_ASSERT_A(tok_expect(TOKTYPE_IDENTIFIER));
    TEST_ASSERT_A(!tok_expect(TOKTYPE_IDENTIFIER));
    TEST_ASSERT_A(tok_expect(TOKTYPE_LPAREN));
    TEST_ASSERT_A(tok_expect(TOKTYPE_RPAREN));
    TEST_ASSERT_A(tok_expect(TOKTYPE_LBRACE));
    TEST_ASSERT_A(tok_expect(TOKTYPE_RBRACE));

    TEST_SUCCESS();
}
