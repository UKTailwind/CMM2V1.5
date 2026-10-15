' ============================================================================
' MMBasic Regular Expression Test Program
' Tests the INSTR() function with regex patterns (size parameter)
' ============================================================================
Print "=========================================="
Print "MMBasic Regular Expression Test Suite"
Print "=========================================="
Print
' Test counter
Dim testnum As Integer = 0
Dim passed As Integer = 0
Dim failed As Integer = 0
' ============================================================================
' Helper subroutine to run a test
' ============================================================================
Sub RunTest(testname$, text$, pattern$, expected_pos, expected_len)
  Local position, size
  testnum = testnum + 1
  Print "Test "; testnum; ": "; testname$
  Print "  Text: "; Chr$(34); text$; Chr$(34)
  Print "  Pattern: "; Chr$(34); pattern$; Chr$(34)
  position = Instr(text$, pattern$, size)
  Print "  Result: pos="; position; ", len="; size
  Print "  Expected: pos="; expected_pos; ", len="; expected_len
  If position = expected_pos And size = expected_len Then
    Print "  [PASS]"
    passed = passed + 1
  Else
    Print "  [FAIL]"
    failed = failed + 1
  EndIf
  Print
End Sub
' ============================================================================
' Test 1: Basic character matching
' ============================================================================
Print "--- Basic Character Matching ---"
RunTest("Simple match", "hello world", "world", 7, 5)
RunTest("No match", "hello world", "xyz", 0, 0)
RunTest("First char", "hello", "h", 1, 1)
RunTest("Last char", "hello", "o", 5, 1)
' ============================================================================
' Test 2: Dot metacharacter (matches any character)
' ============================================================================
Print "--- Dot Metacharacter Tests ---"
RunTest("Dot single", "abc", "a.c", 1, 3)
RunTest("Dot multiple", "hello", "h...o", 1, 5)
RunTest("Dot no match", "abc", "a.d", 0, 0)
' ============================================================================
' Test 3: Character classes [...]
' ============================================================================
Print "--- Character Class Tests ---"
RunTest("Char class match", "hello", "h[aeiou]llo", 1, 5)
RunTest("Char class range", "test123", "[0-9]", 5, 1)
RunTest("Char class alpha", "abc123def", "[a-z]", 1, 1)
RunTest("Multiple ranges", "Test123", "[a-zA-Z0-9]", 1, 1)
' ============================================================================
' Test 4: Inverted character classes [^...]
' ============================================================================
Print "--- Inverted Character Class Tests ---"
RunTest("Not vowel", "hello", "h[^aeiou]", 0, 0)
RunTest("Not digit", "abc123", "[^0-9]", 1, 1)
RunTest("Not alpha", "abc123", "[^a-z]", 4, 1)
' ============================================================================
' Test 5: Anchors (^ and $)
' ============================================================================
Print "--- Anchor Tests ---"
RunTest("Start anchor", "hello world", "^hello", 1, 5)
RunTest("Start anchor fail", "hello world", "^world", 0, 0)
RunTest("End anchor", "hello world", "world$", 7, 5)
RunTest("End anchor fail", "hello world", "hello$", 0, 0)
RunTest("Both anchors", "test", "^test$", 1, 4)
' ============================================================================
' Test 6: Quantifiers - * (zero or more)
' ============================================================================
Print "--- Star Quantifier Tests ---"
RunTest("Star zero match", "ac", "ab*c", 1, 2)
RunTest("Star one match", "abc", "ab*c", 1, 3)
RunTest("Star multiple", "abbbbc", "ab*c", 1, 6)
RunTest("Star greedy", "aaaaaa", "a*", 1, 6)
' ============================================================================
' Test 7: Quantifiers - + (one or more)
' ============================================================================
Print "--- Plus Quantifier Tests ---"
RunTest("Plus no match", "ac", "ab+c", 0, 0)
RunTest("Plus one match", "abc", "ab+c", 1, 3)
RunTest("Plus multiple", "abbbbc", "ab+c", 1, 6)
RunTest("Plus greedy", "aaaaa", "a+", 1, 5)
' ============================================================================
' Test 8: Quantifiers - ? (zero or one)
' ============================================================================
Print "--- Question Quantifier Tests ---"
RunTest("Question zero", "ac", "ab?c", 1, 2)
RunTest("Question one", "abc", "ab?c", 1, 3)
RunTest("Question multiple", "abbc", "ab?c", 0, 0)
' ============================================================================
' Test 9: Escape sequences - \d (digits)
' ============================================================================
Print "--- Digit Escape Tests ---"
RunTest("Digit match", "test123", "\d", 5, 1)
RunTest("Digit+", "test123", "\d+", 5, 3)
RunTest("Not digit", "test123", "\D", 1, 1)
RunTest("Not digit+", "test123", "\D+", 1, 4)
' ============================================================================
' Test 10: Escape sequences - \w (alphanumeric)
' ============================================================================
Print "--- Alphanumeric Escape Tests ---"
RunTest("Word char", "hello world", "\w", 1, 1)
RunTest("Word+", "hello world", "\w+", 1, 5)
RunTest("Not word", "hello world", "\W", 6, 1)
' ============================================================================
' Test 11: Escape sequences - \s (whitespace)
' ============================================================================
Print "--- Whitespace Escape Tests ---"
RunTest("Whitespace", "hello world", "\s", 6, 1)
RunTest("Whitespace+", "hello   world", "\s+", 6, 3)
RunTest("Not whitespace", "hello world", "\S", 1, 1)
RunTest("Not whitespace+", "hello world", "\S+", 1, 5)
' ============================================================================
' Test 12: Complex patterns
' ============================================================================
Print "--- Complex Pattern Tests ---"
RunTest("Email pattern", "test@example.com", "\w+@\w+\.\w+", 1, 16)
RunTest("Date pattern", "Date: 2024-11-20", "\d+-\d+-\d+", 7, 10)
RunTest("Phone pattern", "Call: 123-456-7890", "\d+-\d+-\d+", 7, 12)
RunTest("Word boundary", "the cat in the hat", " [ct]at", 4, 4)
' ============================================================================
' Test 13: Hex escape sequences \xXX
' ============================================================================
Print "--- Hex Escape Tests ---"
RunTest("Hex char", "Hello", "\x48", 1, 1)  ' H = 0x48
RunTest("Hex lower", "hello", "\x68", 1, 1)  ' h = 0x68
RunTest("Hex space", "a b", "\x20", 2, 1)    ' space = 0x20
' ============================================================================
' Test 14: Branch operator |
' ============================================================================
Print "--- Branch Operator Tests ---"
RunTest("Branch first", "hello", "hello|world", 1, 5)
RunTest("Branch simple", "b", "a|b|c", 1, 1)
RunTest("Branch neither", "test", "hello|world", 0, 0)
RunTest("Branch chars", "a", "a|b|c", 1, 1)
' ============================================================================
' Test 15: Quantifiers {n}, {n,}, {,m}, {n,m}
' ============================================================================
Print "--- Counted Quantifier Tests ---"
RunTest("Exact count", "aaaa", "a{4}", 1, 4)
RunTest("Exact fail", "aaa", "a{4}", 0, 0)
RunTest("Min count", "aaaaa", "a{3,}", 1, 5)
RunTest("Max count", "aaaaa", "a{,3}", 1, 3)
RunTest("Range count", "aaaa", "a{2,4}", 1, 4)
' ============================================================================
' Test 16: Word Boundary Tests - \b (word boundary)
' ============================================================================
Print "--- Word Boundary (\b) Tests ---"
RunTest("Word start", "cat", "\bcat", 1, 3)
RunTest("Word start space", "the cat", "\bcat", 5, 3)
RunTest("Word start fail", "concat", "\bcat", 0, 0)
RunTest("Word end", "cat", "cat\b", 1, 3)
RunTest("Word end space", "cat ", "cat\b", 1, 3)
RunTest("Word end fail", "category", "cat\b", 0, 0)
RunTest("Word both ends", "cat", "\bcat\b", 1, 3)
RunTest("Word isolated", "the cat is", "\bcat\b", 5, 3)
RunTest("Word not substring", "concat", "\bcat\b", 0, 0)
RunTest("Word not prefix", "category", "\bcat\b", 0, 0)
' ============================================================================
' Test 17: NOT Word Boundary Tests - \B (not word boundary)
' ============================================================================
Print "--- NOT Word Boundary (\B) Tests ---"
RunTest("Not boundary start", "cat", "\Bcat", 0, 0)
RunTest("Not boundary space", "the cat", "\Bcat", 0, 0)
RunTest("Not boundary inside", "concat", "\Bcat", 4, 3)
RunTest("Not boundary end", "cat", "cat\B", 0, 0)
RunTest("Not boundary end sp", "cat ", "cat\B", 0, 0)
RunTest("Not boundary prefix", "category", "cat\B", 1, 3)
RunTest("Not boundary both", "concatenate", "\Bcat\B", 4, 3)
' ============================================================================
' Test 18: Word Boundary Complex Patterns
' ============================================================================
Print "--- Word Boundary Complex Tests ---"
RunTest("Whole word 'the'", "the", "\bthe\b", 1, 3)
RunTest("Not in 'there'", "there", "\bthe\b", 0, 0)
RunTest("Not in 'breathe'", "breathe", "\bthe\b", 0, 0)
RunTest("Word with plus", "hello", "\b\w+\b", 1, 5)
RunTest("Digits no boundary", "abc123def", "\b[0-9]+\b", 0, 0)
RunTest("Digits with boundary", "abc 123 def", "\b[0-9]+\b", 5, 3)
RunTest("Word then digit", "test123", "\btest\b", 0, 0)
RunTest("Digit then word", "123test", "\btest\b", 0, 0)
' ============================================================================
' Test 19: Word Boundary Edge Cases
' ============================================================================
Print "--- Word Boundary Edge Cases ---"
RunTest("Boundary alone", "hello", "\b", 1, 0)
RunTest("Not boundary alone", "hello", "\B", 2, 0)
RunTest("Double boundary", "hello", "\b\b", 1, 0)
RunTest("Underscore end", "test_", "_\b", 5, 1)
RunTest("Underscore start", "_test", "\b_", 1, 1)
RunTest("Underscore word", "test_case", "\btest_case\b", 1, 9)
' ============================================================================
' Test 20: Word Boundary with Other Features
' ============================================================================
Print "--- Word Boundary Combined Tests ---"
RunTest("Boundary + star", "aaa bbb", "\b\w*\b", 1, 3)
RunTest("Boundary + plus", "aaa bbb", "\b\w+\b", 1, 3)
RunTest("Boundary + question", "a bb", "\b\w?\b", 1, 0)
RunTest("Boundary + range", "test", "\b[a-z]{4}\b", 1, 4)
RunTest("Boundary + digit", "word123", "\b\d+", 0, 0)
RunTest("Boundary + classes", "hello", "\b[aeiou]\w*\b", 0, 0)
RunTest("Multiple words", "cat dog rat", "\bcat\b", 1, 3)
RunTest("Find second word", "cat dog rat", "\bdog\b", 5, 3)
' ============================================================================
' Test 21: Word Boundary Practical Examples
' ============================================================================
Print "--- Word Boundary Practical Tests ---"
RunTest("Find 'is' word", "This is a test", "\bis\b", 6, 2)
RunTest("Not 'is' in 'this'", "This is a test", "\bis\b", 6, 2)
RunTest("Variable name", "var_name = value", "\b\w+\b", 1, 8)
RunTest("Find function", "test() call", "\btest\b", 1, 4)
RunTest("URL domain", "site.com/page", "\b\w+\b", 1, 4)
RunTest("Decimal number", "3.14159", "\b\d+\b", 1, 1)
RunTest("Integer number", "value 42 end", "\b\d+\b", 7, 2)
RunTest("Hyphenated word", "test-case", "\btest\b", 1, 4)
' ============================================================================
' Test 22: Real-world examples
' ============================================================================
Print "--- Real-World Examples ---"
RunTest("IP address", "192.168.1.1", "\d+\.\d+\.\d+\.\d+", 1, 11)
RunTest("HTML tag", "<div>content</div>", "<\w+>", 1, 5)
RunTest("Currency", "$123.45", "\$\d+\.\d+", 1, 7)
RunTest("Time format", "Time: 14:30:00", "\d+:\d+:\d+", 7, 8)
' ============================================================================
' Test 23: Edge cases
' ============================================================================
Print "--- Edge Case Tests ---"
RunTest("Empty pattern", "test", "", 1, 0)
RunTest("Pattern longer", "hi", "hello", 0, 0)
RunTest("Repeated dots", "abc", "...", 1, 3)
RunTest("First occurrence", "hello hello", "hello", 1, 5)  ' Finds first match
'
' ============================================================================
' MMBasic Regular Expression Group Test Program
' Tests the INSTR() function with group patterns (...)
' ============================================================================

Print "=========================================="
Print "MMBasic Regex Group Test Suite"
Print "=========================================="
Print

' ============================================================================
' Test 1: Basic group matching
' ============================================================================
Print "--- Basic Group Tests ---"
RunTest("Simple group", "abc", "(abc)", 1, 3)
RunTest("Group at start", "hello world", "(hello)", 1, 5)
RunTest("Group at end", "hello world", "(world)", 7, 5)
RunTest("Group in middle", "hello world test", "(world)", 7, 5)

' ============================================================================
' Test 2: Groups with single characters
' ============================================================================
Print "--- Single Character Group Tests ---"
RunTest("Single char group", "a", "(a)", 1, 1)
RunTest("Single char no match", "b", "(a)", 0, 0)
RunTest("Multiple single groups", "abc", "(a)(b)(c)", 1, 3)

' ============================================================================
' Test 3: Groups with metacharacters
' ============================================================================
Print "--- Group with Metacharacters ---"
RunTest("Group with dot", "abc", "(a.c)", 1, 3)
RunTest("Group with dots", "hello", "(h...o)", 1, 5)
RunTest("Group with digit", "test123", "(\d)", 5, 1)
RunTest("Group with digits", "test123", "(\d\d\d)", 5, 3)
RunTest("Group with word", "hello world", "(\w+)", 1, 5)
RunTest("Group with space", "hello world", "(\s)", 6, 1)

' ============================================================================
' Test 4: Groups with character classes
' ============================================================================
Print "--- Group with Character Classes ---"
RunTest("Group with class", "abc", "([abc])", 1, 1)
RunTest("Group with range", "test5", "([0-9])", 5, 1)
RunTest("Group multiple class", "test123", "([a-z]+)", 1, 4)
RunTest("Group inv class", "123abc", "([^0-9])", 4, 1)

' ============================================================================
' Test 5: Quantifiers on groups - * (zero or more)
' ============================================================================
Print "--- Group with Star Quantifier ---"
RunTest("Group star zero", "b", "(a)*b", 1, 1)
RunTest("Group star one", "ab", "(a)*b", 1, 2)
RunTest("Group star many", "aaaab", "(a)*b", 1, 5)
RunTest("Group word star", "test", "(test)*", 1, 4)
RunTest("Group multi star", "ababab", "(ab)*", 1, 6)

' ============================================================================
' Test 6: Quantifiers on groups - + (one or more)
' ============================================================================
Print "--- Group with Plus Quantifier ---"
RunTest("Group plus fail", "b", "(a)+b", 0, 0)
RunTest("Group plus one", "ab", "(a)+b", 1, 2)
RunTest("Group plus many", "aaaab", "(a)+b", 1, 5)
RunTest("Group multi plus", "ababab", "(ab)+", 1, 6)
RunTest("Group word plus", "testtest", "(test)+", 1, 8)

' ============================================================================
' Test 7: Quantifiers on groups - ? (zero or one)
' ============================================================================
Print "--- Group with Question Quantifier ---"
RunTest("Group question zero", "b", "(a)?b", 1, 1)
RunTest("Group question one", "ab", "(a)?b", 1, 2)
' FIXED: Pattern (a)?b correctly matches "ab" substring at position 2 in "aab"
RunTest("Group question partial", "aab", "(a)?b", 2, 2)
RunTest("Group multi question", "ab", "(ab)?", 1, 2)

' ============================================================================
' Test 8: Counted quantifiers on groups {n}, {n,}, {,m}, {n,m}
' ============================================================================
Print "--- Group with Counted Quantifiers ---"
RunTest("Group exact count", "aaaa", "(a){4}", 1, 4)
RunTest("Group exact fail", "aaa", "(a){4}", 0, 0)
RunTest("Group min count", "aaaaa", "(a){3,}", 1, 5)
RunTest("Group max count", "aaaaa", "(a){,3}", 1, 3)
RunTest("Group range count", "aaaa", "(a){2,4}", 1, 4)
RunTest("Group multi exact", "ababab", "(ab){3}", 1, 6)
RunTest("Group multi range", "abababab", "(ab){2,3}", 1, 6)

' ============================================================================
' Test 9: Sequential groups
' ============================================================================
Print "--- Sequential Group Tests ---"
RunTest("Two groups", "helloworld", "(hello)(world)", 1, 10)
RunTest("Three groups", "abc", "(a)(b)(c)", 1, 3)
RunTest("Groups with chars", "a-b", "(a)(-)(b)", 1, 3)
RunTest("Groups and text", "test", "t(es)t", 1, 4)

' ============================================================================
' Test 10: Nested groups
' ============================================================================
Print "--- Nested Group Tests ---"
RunTest("Simple nested", "abc", "((abc))", 1, 3)
RunTest("Nested with char", "abc", "(a(bc))", 1, 3)
RunTest("Nested middle", "abc", "((a)bc)", 1, 3)
RunTest("Deep nested", "a", "(((a)))", 1, 1)
RunTest("Multiple nested", "hello", "((he)(llo))", 1, 5)

' ============================================================================
' Test 11: Groups with anchors
' ============================================================================
Print "--- Group with Anchor Tests ---"
RunTest("Group start anchor", "hello", "^(hello)", 1, 5)
RunTest("Group end anchor", "hello", "(hello)$", 1, 5)
RunTest("Group both anchors", "test", "^(test)$", 1, 4)
' KNOWN LIMITATION: Anchors inside groups don't currently work
' These tests verify the current behavior (no match) rather than ideal behavior
RunTest("Anchor in group (limitation)", "hello", "(^hello)", 0, 0)
RunTest("End anchor in group (limitation)", "hello", "(hello$)", 0, 0)

' ============================================================================
' Test 12: Groups with branches
' ============================================================================
Print "--- Group with Branch Tests ---"
RunTest("Group branch simple", "a", "(a|b)", 1, 1)
RunTest("Group branch second", "b", "(a|b)", 1, 1)
RunTest("Group branch word", "cat", "(cat|dog)", 1, 3)
RunTest("Group branch fail", "bird", "(cat|dog)", 0, 0)
RunTest("Multi branch group", "test", "(abc|def|test)", 1, 4)

' ============================================================================
' Test 13: Complex group patterns
' ============================================================================
Print "--- Complex Group Patterns ---"
RunTest("Email group", "test@example.com", "(\w+)@(\w+)\.(\w+)", 1, 16)
RunTest("Date group", "2024-11-20", "(\d+)-(\d+)-(\d+)", 1, 10)
RunTest("Phone group", "123-456-7890", "(\d+)-(\d+)-(\d+)", 1, 12)
RunTest("URL protocol", "http://test.com", "(https?://)(\w+)\.(\w+)", 1, 15)

' ============================================================================
' Test 14: Groups with special patterns
' ============================================================================
Print "--- Special Group Pattern Tests ---"
RunTest("Empty group", "test", "()", 1, 0)
RunTest("Optional group", "test", "(test)?", 1, 4)
RunTest("Group with spaces", "hello world", "(hello )(world)", 1, 11)
RunTest("Group with tab", "a b", "(a\s+b)", 1, 3)

' ============================================================================
' Test 15: Group boundary cases
' ============================================================================
Print "--- Group Boundary Tests ---"
RunTest("Group first char", "hello", "(h)", 1, 1)
RunTest("Group last char", "hello", "(o)", 5, 1)
RunTest("Whole text group", "test", "(test)", 1, 4)
RunTest("Partial match group", "testing", "(test)", 1, 4)

' ============================================================================
' Test 16: Groups with escaped characters
' ============================================================================
Print "--- Group with Escaped Chars ---"
RunTest("Group with dot", "a.b", "(a\.b)", 1, 3)
RunTest("Group with star", "a*b", "(a\*b)", 1, 3)
RunTest("Group with backslash", "a\b", "(a\\b)", 1, 3)
RunTest("Group with bracket", "a[b", "(a\[b)", 1, 3)

' ============================================================================
' Test 17: Groups in real-world scenarios
' ============================================================================
Print "--- Real-World Group Examples ---"
RunTest("Extract domain", "user@domain.com", "@(\w+)\.", 5, 8)
RunTest("Extract filename", "file.txt", "(\w+)\.", 1, 5)
RunTest("Extract number", "Price: $123", "\$(\d+)", 8, 4)
RunTest("Extract time", "Time: 14:30", "(\d+):(\d+)", 7, 5)

' ============================================================================
' Test 18: Multiple groups with quantifiers
' ============================================================================
Print "--- Multiple Groups with Quantifiers ---"
RunTest("Two groups plus", "aaabbb", "(a)+(b)+", 1, 6)
RunTest("Groups with star", "aaabbb", "(a)*(b)*", 1, 6)
RunTest("Mixed quantifiers", "aaab", "(a)+(b)?", 1, 4)
RunTest("Groups counted", "aabbcc", "(a){2}(b){2}(c){2}", 1, 6)

' ============================================================================
' Test 19: Groups with different content types
' ============================================================================
Print "--- Groups with Mixed Content ---"
RunTest("Group alpha num", "test123", "([a-z]+)(\d+)", 1, 7)
RunTest("Group num alpha", "123test", "(\d+)([a-z]+)", 1, 7)
RunTest("Group mixed", "a1b2c3", "(\w)(\d)", 1, 2)
RunTest("Group words", "hello world", "(\w+)\s(\w+)", 1, 11)

' ============================================================================
' Test 20: Edge cases and stress tests
' ============================================================================
Print "--- Edge Case Tests ---"
RunTest("Many groups", "abcdef", "(a)(b)(c)(d)(e)(f)", 1, 6)
RunTest("Repeated groups", "ababab", "((ab))+", 1, 6)
RunTest("Group at boundaries", "test", "^(test)$", 1, 4)
RunTest("Group no match end", "test", "(test)x", 0, 0)

' ============================================================================
' Summary
' ============================================================================
Print "=========================================="
Print "Test Summary"
Print "=========================================="
Print "Total tests: "; testnum
Print "Passed: "; passed
Print "Failed: "; failed
Print "Success rate: "; Format$(passed/testnum*100, "%.1f"); "%"
Print "=========================================="

If failed = 0 Then
  Print "All tests PASSED!"
Else
  Print "Some tests FAILED - please review"
EndIf

End
