---
trigger: always_on
description: Instructs the AI to use PROJECT_MEMORY.md for persistent project-specific context across conversations.
---

## Project Memory

**Single source of truth:** `G:\GitHub\Solace-Synth\.agent\synth-project-memory.md`

Read this file in full at the start of every conversation. It is the authoritative rolling record of all decisions, architecture, bugs fixed, and current status.

**Before editing the memory file**, read the update protocol: `.agent/rules/memory-update-protocol.md`

Key rules:
- Search for ALL mentions of an item before updating — no contradictions allowed
- Phase logs are frozen history — don't edit completed phases
- Mark resolved items as `✅ RESOLVED` everywhere, not just in one place
- Anshul is the sole decision-maker — AI reviews are input, not authority
- Use absolute dates, not relative ones
- Don't rewrite from scratch — targeted edits only