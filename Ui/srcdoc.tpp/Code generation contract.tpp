Code generation contract

Generated code is derived from the Designer model.

The contract for V1 is:

- preserve the runtime layout tree
- preserve fixed sizing, Fit, Expand, min/max, and alignment decisions
- preserve explicit theme overrides when they are enabled
- do not emit stale or legacy property names unless they are used as fallback
- keep the code readable enough to regenerate and inspect later

The generated source is not where application-specific logic should live.
User code should sit around it, not inside it.
