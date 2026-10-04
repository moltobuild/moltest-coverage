# Dependencies

## Policy
Same as moltest:
- Exact versions only; no install or post-install scripts.
- Zero runtime dependencies beyond moltest and libc.
- External tools (gcov, llvm-cov) are found on the system, never bundled.
- Any new dependency needs an ADR and the maintainer's approval.
- `Molto.lock` is committed whenever one exists.

## Runtime
| Name | Why |
|---|---|
| moltest | the runner this plugs into (consumer's own copy; see ARCHITECTURE open questions) |

## External tools (not linked)
| Tool | Why |
|---|---|
| gcov (GCC) or `llvm-cov gcov` (Clang) | turns `.gcda`/`.gcno` into annotated text |
| molto, pickup | build, test, fmt, lint |
