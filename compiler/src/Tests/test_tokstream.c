#include "testing.h"
#include "tokstream.h"

DECLARE_TEST(tokstream)
{
    const char *text = "int main() {}";

    init_errors("[test_lexer]", text);
    init_lexer();

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
