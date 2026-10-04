# Security

## Threat model
Runs inside the consumer's test binary with the developer's or CI's
privileges. Inputs: `moltest-coverage.toml`, `.gcda`/`.gcno` files and the text
an external gcov tool prints. Outputs: files written at configured paths.

## Rules
- The config file is parsed with bounded buffers; unknown keys are refused, not ignored.
- The gcov tool is started with an argument vector (no shell), from a temp directory.
- Output paths are relative to the project root; absolute paths and `..` are refused.
- `.gcov` text is untrusted input: every line is length-checked.
- Nothing is written outside the temp directory and the configured outputs.

## Per-feature checklist
- [ ] Input from config, files and tool output is length-checked and validated
- [ ] No shell; argv only
- [ ] Output paths confined to the project root
- [ ] Temp directory created atomically and removed
