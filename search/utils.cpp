#include "search/utils.hpp"

#include <cctype>

namespace search
{
size_t GetMaxErrorsForToken(strings::UniString const & token)
{
  bool const digitsOnly = std::all_of(token.begin(), token.end(), ::isdigit);
  if (digitsOnly)
    return 0;
  if (token.size() < 4)
    return 0;
  if (token.size() < 8)
    return 1;
  return 2;
}

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s)
{
  // In search we use LevenshteinDFAs for fuzzy matching. But due to
  // performance reasons, we assume that the first letter is always
  // correct.
  return strings::LevenshteinDFA(s, 1 /* prefixCharsToKeep */, GetMaxErrorsForToken(s));
}
}  // namespace search
