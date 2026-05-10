# Next steps

- No code follow-up is currently required for this issue.
- If more indexed draw assertions fail later, check whether they are still hardcoding the old 16-bit index assumption.

## Risks

- Any other tests or docs that still assume `uint16` index formats may need to be updated to reflect the current 32-bit `DvzIndex` type.
